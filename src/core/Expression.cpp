#include "Expression.hpp"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <set>
#include <string_view>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace rationalstudio {

    namespace {

        constexpr double pi =
        3.14159265358979323846264338327950288;

        constexpr double tau = 2 * pi;

        constexpr double euler =
        2.71828182845904523536028747135266250;

        constexpr double nan =
        std::numeric_limits<double>::quiet_NaN();

        enum class Type {
            Number,
            Name,
            Plus,
            Minus,
            Star,
            Slash,
            Percent,
            Power,
            Fact,
            Open,
            Close,
            Comma,
            End
        };

        struct Token {

            Type type;
            std::string text;
            double number = 0;
            std::size_t position = 0;
        };

        std::string asciiMath(std::string s) {

            const std::vector<
            std::pair<std::string, std::string>
            > substitutions = {

                {"π", "pi"},
                {"τ", "tau"},
                {"×", "*"},
                {"÷", "/"},
                {"−", "-"},
                {"√", "sqrt"},
                {"²", "^2"}
            };

            for (const auto& [from, to] : substitutions) {

                std::size_t start = 0;

                while (
                    (start = s.find(from, start)) !=
                    std::string::npos
                ) {

                    s.replace(
                        start,
                        from.size(),
                              to
                    );

                    start += to.size();
                }
            }

            return s;
        }

        std::vector<Token> tokenize(
            const std::string& source
        ) {

            const std::string s = asciiMath(source);

            if (s.size() > 2048) {

                throw std::runtime_error(
                    "Expression exceeds 2048 characters"
                );
            }

            std::vector<Token> tokens;

            std::size_t i = 0;

            while (i < s.size()) {

                const unsigned char c =
                static_cast<unsigned char>(s[i]);

                if (
                    c == ' ' ||
                    c == '\t' ||
                    c == '\n' ||
                    c == '\r'
                ) {

                    ++i;
                    continue;
                }

                if (tokens.size() >= 1024) {

                    throw std::runtime_error(
                        "Too many expression tokens"
                    );
                }

                const std::size_t start = i;

                // Numeric literals, including
                // scientific notation such as 1.2e-3.

                if (
                    (c >= '0' && c <= '9') ||
                    c == '.'
                ) {

                    char* end = nullptr;

                    errno = 0;

                    const double d = std::strtod(
                        s.c_str() + i,
                                                 &end
                    );

                    if (
                        end == s.c_str() + i ||
                        errno == ERANGE ||
                        !std::isfinite(d)
                    ) {

                        throw std::runtime_error(
                            "Invalid or out-of-range number at position " +
                            std::to_string(i + 1)
                        );
                    }

                    i = static_cast<std::size_t>(
                        end - s.c_str()
                    );

                    tokens.push_back({
                        Type::Number,
                        s.substr(start, i - start),
                                     d,
                                     start
                    });

                    continue;
                }

                // Identifiers and function names.

                if (
                    (c >= 'a' && c <= 'z') ||
                    (c >= 'A' && c <= 'Z') ||
                    c == '_'
                ) {

                    while (i < s.size()) {

                        const unsigned char v =
                        static_cast<unsigned char>(s[i]);

                        if (
                            (v >= 'a' && v <= 'z') ||
                            (v >= 'A' && v <= 'Z') ||
                            (v >= '0' && v <= '9') ||
                            v == '_'
                        ) {

                            ++i;

                        } else {

                            break;
                        }
                    }

                    auto name = s.substr(
                        start,
                        i - start
                    );

                    std::transform(
                        name.begin(),
                                   name.end(),
                                   name.begin(),

                                   [](unsigned char a) {

                                       return static_cast<char>(
                                           std::tolower(a)
                                       );
                                   }
                    );

                    tokens.push_back({
                        Type::Name,
                        name,
                        0,
                        start
                    });

                    continue;
                }

                ++i;

                Type type;

                switch (c) {

                    case '+':
                        type = Type::Plus;
                        break;

                    case '-':
                        type = Type::Minus;
                        break;

                    case '*':
                        type = Type::Star;
                        break;

                    case '/':
                        type = Type::Slash;
                        break;

                    case '%':
                        type = Type::Percent;
                        break;

                    case '^':
                        type = Type::Power;
                        break;

                    case '!':
                        type = Type::Fact;
                        break;

                    case '(':
                        type = Type::Open;
                        break;

                    case ')':
                        type = Type::Close;
                        break;

                    case ',':
                        type = Type::Comma;
                        break;

                    default:

                        throw std::runtime_error(
                            "Unknown character at position " +
                            std::to_string(start + 1)
                        );
                }

                tokens.push_back({
                    type,
                    s.substr(start, 1),
                                 0,
                                 start
                });
            }

            tokens.push_back({
                Type::End,
                {},
                0,
                s.size()
            });

            return tokens;
        }

        struct Node {

            enum class Kind {
                Value,
                X,
                Ans,
                Named,
                Unary,
                Binary,
                Fact,
                Function
            } kind;

            double number = 0;

            Type op = Type::End;

            std::string function;

            std::vector<
            std::unique_ptr<Node>
            > args;

            explicit Node(Kind kind)
            : kind(kind) {
            }
        };

        const std::unordered_map<
        std::string,
        unsigned
        >& functions() {

            static const std::unordered_map<
            std::string,
            unsigned
            > map = {

                {"sin", 1},
                {"cos", 1},
                {"tan", 1},

                {"asin", 1},
                {"acos", 1},
                {"atan", 1},

                {"sinh", 1},
                {"cosh", 1},
                {"tanh", 1},

                {"sqrt", 1},
                {"cbrt", 1},

                {"ln", 1},
                {"log", 1},
                {"log10", 1},
                {"log2", 1},

                {"exp", 1},
                {"abs", 1},
                {"floor", 1},
                {"ceil", 1},
                {"round", 1},
                {"sign", 1},

                {"deg", 1},
                {"rad", 1},

                {"sec", 1},
                {"csc", 1},
                {"cot", 1},

                {"min", 2},
                {"max", 2},
                {"pow", 2},
                {"atan2", 2},
                {"hypot", 2},
                {"mod", 2}
            };

            return map;
        }

        // Keep identifier rules identical for declarations, bindings and
        // parser references; lexer already converts expression names to lower.
        bool normalizeUserIdentifier(std::string_view input, std::string& out,
                                     std::string* error = nullptr) {
            const auto reject = [&](const char* why) {
                if (error) *error = why;
                return false;
            };
            if (input.empty() || input.size() > 64)
                return reject("Identifier length must be between 1 and 64 ASCII characters");
            out.clear();
            out.reserve(input.size());
            for (std::size_t i = 0; i < input.size(); ++i) {
                const unsigned char c = static_cast<unsigned char>(input[i]);
                const bool letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
                if (!letter && c != '_' && (i == 0 || c < '0' || c > '9'))
                    return reject("Identifier must match [A-Za-z_][A-Za-z0-9_]*");
                out.push_back(letter ? static_cast<char>(std::tolower(c))
                                     : static_cast<char>(c));
            }
            if (out == "x" || out == "ans" || out == "pi" || out == "e" ||
                out == "tau" || functions().find(out) != functions().end())
                return reject("Identifier is reserved by the expression language");
            if (error) error->clear();
            return true;
        }

        class Parser {
        public:

            Parser(const std::string& s, const VariableScope* scope)
            : tokens_(tokenize(s)), scope_(scope) {
            }

            std::unique_ptr<Node> parse() {

                auto value = expression(0);

                if (peek().type != Type::End) {
                    fail("Unexpected token");
                }

                return value;
            }

            bool dependsOnX() const noexcept { return usesX_; }
            bool dependsOnAns() const noexcept { return usesAns_; }
            const std::set<std::string>& namedVariables() const noexcept {
                return namedVariables_;
            }

        private:

            std::vector<Token> tokens_;

            std::size_t index_ = 0;
            unsigned nodes_ = 0;

            bool usesX_ = false;
            bool usesAns_ = false;
            const VariableScope* scope_ = nullptr;
            std::set<std::string> namedVariables_;

            const Token& peek() const {
                return tokens_.at(index_);
            }

            bool take(Type t) {

                if (peek().type == t) {

                    ++index_;
                    return true;
                }

                return false;
            }

            [[noreturn]]
            void fail(const std::string& message) const {

                throw std::runtime_error(
                    message +
                    " at position " +
                    std::to_string(
                        peek().position + 1
                    )
                );
            }

            std::unique_ptr<Node> node(Node::Kind kind) {

                if (++nodes_ > 512) {
                    fail("Expression too complex");
                }

                return std::make_unique<Node>(kind);
            }

            // Conventional precedence:
            //
            // x^y is right-associative.
            // -2^2 == -(2^2).

            std::unique_ptr<Node> expression(
                unsigned depth
            ) {

                auto lhs = term(depth + 1);

                while (
                    peek().type == Type::Plus ||
                    peek().type == Type::Minus
                ) {

                    const Type op =
                    tokens_[index_++].type;

                    auto rhs = term(depth + 1);

                    auto n = node(Node::Kind::Binary);

                    n->op = op;

                    n->args.push_back(
                        std::move(lhs)
                    );

                    n->args.push_back(
                        std::move(rhs)
                    );

                    lhs = std::move(n);
                }

                return lhs;
            }

            std::unique_ptr<Node> term(
                unsigned depth
            ) {

                auto lhs = unary(depth + 1);

                for (;;) {

                    Type op = peek().type;

                    bool implicit = (
                        op == Type::Open ||
                        op == Type::Name
                    );

                    if (
                        op != Type::Star &&
                        op != Type::Slash &&
                        op != Type::Percent &&
                        !implicit
                    ) {

                        break;
                    }

                    if (implicit) {

                        op = Type::Star;

                    } else {

                        ++index_;
                    }

                    auto rhs = unary(depth + 1);

                    auto n = node(Node::Kind::Binary);

                    n->op = op;

                    n->args.push_back(
                        std::move(lhs)
                    );

                    n->args.push_back(
                        std::move(rhs)
                    );

                    lhs = std::move(n);
                }

                return lhs;
            }

            std::unique_ptr<Node> unary(
                unsigned depth
            ) {

                if (depth > 100) {
                    fail("Expression nesting too deep");
                }

                if (
                    peek().type == Type::Plus ||
                    peek().type == Type::Minus
                ) {

                    Type op = tokens_[index_++].type;

                    auto n = node(Node::Kind::Unary);

                    n->op = op;

                    n->args.push_back(
                        unary(depth + 1)
                    );

                    return n;
                }

                return power(depth + 1);
            }

            std::unique_ptr<Node> power(
                unsigned depth
            ) {

                auto base = postfix(depth + 1);

                if (take(Type::Power)) {

                    auto exponent = unary(depth + 1);

                    auto n = node(Node::Kind::Binary);

                    n->op = Type::Power;

                    n->args.push_back(
                        std::move(base)
                    );

                    n->args.push_back(
                        std::move(exponent)
                    );

                    return n;
                }

                return base;
            }

            std::unique_ptr<Node> postfix(
                unsigned depth
            ) {

                auto n = primary(depth + 1);

                while (take(Type::Fact)) {

                    auto f = node(Node::Kind::Fact);

                    f->args.push_back(
                        std::move(n)
                    );

                    n = std::move(f);
                }

                return n;
            }

            std::unique_ptr<Node> primary(
                unsigned depth
            ) {

                if (depth > 100) {
                    fail("Expression nesting too deep");
                }

                // Numeric literals.

                if (peek().type == Type::Number) {

                    auto n = node(Node::Kind::Value);

                    n->number =
                    tokens_[index_++].number;

                    return n;
                }

                // Constants, variables, functions.

                if (peek().type == Type::Name) {

                    const std::string name =
                    tokens_[index_++].text;

                    if (name == "x") {

                        usesX_ = true;

                        return node(Node::Kind::X);
                    }

                    if (name == "ans") {

                        usesAns_ = true;
                        return node(Node::Kind::Ans);
                    }

                    if (
                        name == "pi" ||
                        name == "e" ||
                        name == "tau"
                    ) {

                        auto n = node(Node::Kind::Value);

                        n->number =
                        name == "pi"
                        ? pi
                        : (
                            name == "e"
                            ? euler
                            : tau
                        );

                        return n;
                    }

                    // Named values must be explicitly declared. Do not
                    // reinterpret builtin function calls as variable names.
                    if (scope_ && scope_->contains(name)) {
                        namedVariables_.insert(name);
                        auto n = node(Node::Kind::Named);
                        n->function = name;
                        return n;
                    }

                    const auto it =
                    functions().find(name);

                    if (it == functions().end()) {

                        fail(
                            "Unknown identifier '" +
                            name +
                            "'"
                        );
                    }

                    if (!take(Type::Open)) {

                        fail(
                            "Function '" +
                            name +
                            "' requires parentheses"
                        );
                    }

                    auto n = node(Node::Kind::Function);

                    n->function = name;

                    if (!take(Type::Close)) {

                        do {

                            n->args.push_back(
                                expression(depth + 1)
                            );

                        } while (take(Type::Comma));

                        if (!take(Type::Close)) {

                            fail("Expected ')'");
                        }
                    }

                    if (n->args.size() != it->second) {

                        fail(
                            "Function '" +
                            name +
                            "' expects " +
                            std::to_string(it->second) +
                            " argument(s)"
                        );
                    }

                    return n;
                }

                // Parenthesized expressions.

                if (take(Type::Open)) {

                    auto n = expression(depth + 1);

                    if (!take(Type::Close)) {
                        fail("Expected ')'");
                    }

                    return n;
                }

                fail(
                    "Expected a number, variable, or '('"
                );
            }
        };

        double eval(
            const Node& n,
            double x,
            double ans,
            AngleMode mode,
            const VariableBindings* bindings
        ) {

            const auto arg = [&](unsigned i) {

                return eval(
                    *n.args[i],
                    x,
                    ans,
                    mode,
                    bindings
                );
            };

            const auto toRad = [&](double v) {

                return mode == AngleMode::Degrees
                ? v * pi / 180
                : v;
            };

            const auto fromRad = [&](double v) {

                return mode == AngleMode::Degrees
                ? v * 180 / pi
                : v;
            };

            switch (n.kind) {

                case Node::Kind::Value:
                    return n.number;

                case Node::Kind::X:
                    return x;

                case Node::Kind::Ans:
                    return ans;

                case Node::Kind::Named: {
                    double value = nan;
                    return bindings && bindings->lookup(n.function, value)
                        ? value : nan;
                }

                case Node::Kind::Unary:
                    return n.op == Type::Minus
                    ? -arg(0)
                    : arg(0);

                case Node::Kind::Fact: {

                    const double v = arg(0);

                    if (
                        !std::isfinite(v) ||
                        v < 0 ||
                        v > 170 ||
                        std::abs(v - std::round(v)) > 1e-10
                    ) {

                        return nan;
                    }

                    return std::tgamma(v + 1);
                }

                case Node::Kind::Binary: {

                    const double a = arg(0);
                    const double b = arg(1);

                    switch (n.op) {

                        case Type::Plus:
                            return a + b;

                        case Type::Minus:
                            return a - b;

                        case Type::Star:
                            return a * b;

                        case Type::Slash:
                            return b == 0
                            ? nan
                            : a / b;

                        case Type::Percent:
                            return b == 0
                            ? nan
                            : std::fmod(a, b);

                        case Type::Power:
                            return std::pow(a, b);

                        default:
                            return nan;
                    }
                }

                        case Node::Kind::Function: {

                            const std::string& f = n.function;

                            const double a = arg(0);

                            if (f == "sin") {
                                return std::sin(toRad(a));
                            }

                            if (f == "cos") {
                                return std::cos(toRad(a));
                            }

                            if (f == "tan") {

                                return std::abs(
                                    std::cos(toRad(a))
                                ) < 1e-13
                                ? nan
                                : std::tan(toRad(a));
                            }

                            if (f == "sec") {

                                return std::abs(
                                    std::cos(toRad(a))
                                ) < 1e-13
                                ? nan
                                : 1 / std::cos(toRad(a));
                            }

                            if (f == "csc") {

                                return std::abs(
                                    std::sin(toRad(a))
                                ) < 1e-13
                                ? nan
                                : 1 / std::sin(toRad(a));
                            }

                            if (f == "cot") {

                                return std::abs(
                                    std::sin(toRad(a))
                                ) < 1e-13
                                ? nan
                                : 1 / std::tan(toRad(a));
                            }

                            if (f == "asin") {
                                return fromRad(std::asin(a));
                            }

                            if (f == "acos") {
                                return fromRad(std::acos(a));
                            }

                            if (f == "atan") {
                                return fromRad(std::atan(a));
                            }

                            if (f == "sinh") {
                                return std::sinh(a);
                            }

                            if (f == "cosh") {
                                return std::cosh(a);
                            }

                            if (f == "tanh") {
                                return std::tanh(a);
                            }

                            if (f == "sqrt") {
                                return std::sqrt(a);
                            }

                            if (f == "cbrt") {
                                return std::cbrt(a);
                            }

                            if (f == "ln") {
                                return std::log(a);
                            }

                            if (f == "log" || f == "log10") {
                                return std::log10(a);
                            }

                            if (f == "log2") {
                                return std::log2(a);
                            }

                            if (f == "exp") {
                                return std::exp(a);
                            }

                            if (f == "abs") {
                                return std::abs(a);
                            }

                            if (f == "floor") {
                                return std::floor(a);
                            }

                            if (f == "ceil") {
                                return std::ceil(a);
                            }

                            if (f == "round") {
                                return std::round(a);
                            }

                            if (f == "sign") {
                                return (a > 0) - (a < 0);
                            }

                            if (f == "deg") {
                                return a * 180 / pi;
                            }

                            if (f == "rad") {
                                return a * pi / 180;
                            }

                            // Two-argument functions.

                            const double b = arg(1);

                            if (f == "min") {

                                return std::isnan(a) || std::isnan(b)
                                ? nan
                                : std::fmin(a, b);
                            }

                            if (f == "max") {

                                return std::isnan(a) || std::isnan(b)
                                ? nan
                                : std::fmax(a, b);
                            }

                            if (f == "pow") {
                                return std::pow(a, b);
                            }

                            if (f == "atan2") {

                                return fromRad(
                                    std::atan2(a, b)
                                );
                            }

                            if (f == "hypot") {
                                return std::hypot(a, b);
                            }

                            if (f == "mod") {

                                return b == 0
                                ? nan
                                : std::fmod(a, b);
                            }

                            return nan;
                        }
            }

            return nan;
        }

    } // namespace

    bool VariableScope::declare(std::string_view name, std::string* error) {
        std::string normalized;
        if (!normalizeUserIdentifier(name, normalized, error)) return false;
        if (!names_.insert(normalized).second) {
            if (error) *error = "Duplicate identifier '" + normalized + "'";
            return false;
        }
        if (error) error->clear();
        return true;
    }

    bool VariableScope::contains(std::string_view name) const {
        std::string normalized;
        return normalizeUserIdentifier(name, normalized) &&
               names_.find(normalized) != names_.end();
    }

    bool VariableBindings::set(std::string_view name, double value) {
        std::string normalized;
        if (!std::isfinite(value) || !normalizeUserIdentifier(name, normalized)) return false;
        values_.insert_or_assign(std::move(normalized), value);
        return true;
    }

    bool VariableBindings::erase(std::string_view name) {
        std::string normalized;
        return normalizeUserIdentifier(name, normalized) && values_.erase(normalized) != 0;
    }

    bool VariableBindings::lookup(std::string_view name, double& value) const {
        // Internal named nodes use canonical lowercase names. Transparent
        // map lookup avoids allocating a new key on every curve sample.
        const auto it = values_.find(name);
        if (it != values_.end() && std::isfinite(it->second)) {
            value = it->second;
            return true;
        }
        std::string normalized;
        if (!normalizeUserIdentifier(name, normalized)) return false;
        const auto canonical = values_.find(normalized);
        if (canonical == values_.end() || !std::isfinite(canonical->second)) return false;
        value = canonical->second;
        return true;
    }

    struct Expression::Impl {

        std::unique_ptr<Node> root;

        std::string error;

        bool hasX = false;
        bool hasAns = false;
        std::vector<std::string> referenced;
    };

    Expression::Expression(const std::string& source)
        : Expression(source, VariableScope{}) {}

    Expression::Expression(const std::string& source, const VariableScope& scope)
        : impl_(std::make_unique<Impl>()) {
        try {
            Parser parser(source, &scope);
            impl_->root = parser.parse();
            impl_->hasX = parser.dependsOnX();
            impl_->hasAns = parser.dependsOnAns();
            impl_->referenced.assign(parser.namedVariables().begin(),
                                     parser.namedVariables().end());
        } catch (const std::exception& exception) {
            impl_->error = exception.what();
        }
    }

    Expression::~Expression() = default;

    Expression::Expression(Expression&&) noexcept = default;

    Expression& Expression::operator=(
        Expression&&
    ) noexcept = default;

    bool Expression::valid() const noexcept {

        return impl_ && impl_->root != nullptr;
    }

    const std::string& Expression::error() const noexcept {

        static const std::string movedFrom =
        "Expression has been moved";

                        return impl_
                        ? impl_->error
                        : movedFrom;
    }

    bool Expression::dependsOnX() const noexcept {

        return impl_ && impl_->hasX;
    }

    bool Expression::dependsOnAns() const noexcept {
        return impl_ && impl_->hasAns;
    }

    const std::vector<std::string>& Expression::referencedVariables() const noexcept {
        static const std::vector<std::string> empty;
        return impl_ ? impl_->referenced : empty;
    }

    double Expression::evaluate(double x, double ans, AngleMode angle) const noexcept {
        return evaluateImpl(x, ans, angle, nullptr);
    }

    double Expression::evaluate(double x, double ans, AngleMode angle,
                                const VariableBindings& bindings) const noexcept {
        return evaluateImpl(x, ans, angle, &bindings);
    }

    double Expression::evaluateImpl(double x, double ans, AngleMode angle,
                                    const VariableBindings* bindings) const noexcept {
        if (!valid()) return nan;
        try {
            const double value = eval(*impl_->root, x, ans, angle, bindings);
            return std::isfinite(value) ? value : nan;
        } catch (...) {
            return nan;
        }
    }

} // namespace rationalstudio
