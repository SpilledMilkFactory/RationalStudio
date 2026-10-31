#include "ComplexMath.hpp"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace rationalstudio {

bool finiteComplex(ComplexValue v) noexcept {
    return std::isfinite(v.real()) && std::isfinite(v.imag());
}

const char* complexStatusName(ComplexStatus s) noexcept {
    switch (s) {
    case ComplexStatus::Success: return "success";
    case ComplexStatus::InvalidExpression: return "invalid expression";
    case ComplexStatus::UndefinedValue: return "undefined value";
    case ComplexStatus::UnsupportedOperation: return "unsupported operation";
    }
    return "unknown";
}

std::string formatComplex(ComplexValue v, int precision) {
    if (!finiteComplex(v)) return "undefined";
    if (precision < 1 || precision > 17) return "invalid precision";
    const auto number = [precision](double x) {
        std::ostringstream out;
        out << std::setprecision(precision) << (x == 0.0 ? 0.0 : x);
        return out.str();
    };
    const double re = v.real(), im = v.imag();
    if (im == 0.0) return number(re);
    const double magnitude = std::abs(im);
    const std::string imag = magnitude == 1.0 ? "i" : number(magnitude) + "i";
    if (re == 0.0) return im < 0.0 ? "-" + imag : imag;
    return number(re) + (im < 0.0 ? " - " : " + ") + imag;
}

namespace {
constexpr double pi = std::numbers::pi;
constexpr double degToRad = pi / 180.0;
constexpr double radToDeg = 180.0 / pi;
constexpr std::size_t maxNodes = 512;
constexpr unsigned maxDepth = 64;

enum class Kind { Number, Name, Plus, Minus, Mul, Div, Power,
                  Open, Close, Comma, End };
struct Token {
    Kind kind;
    std::string text;
    double number = 0.0;
    std::size_t location = 0;
};

[[nodiscard]] std::vector<Token> lex(const std::string& text) {
    if (text.size() > 2048) throw std::runtime_error("Expression exceeds 2048 bytes");
    std::vector<Token> out;
    std::size_t pos = 0;
    while (pos < text.size()) {
        const unsigned char c = static_cast<unsigned char>(text[pos]);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { ++pos; continue; }
        if (out.size() >= 1024) throw std::runtime_error("Expression has too many tokens");
        const std::size_t start = pos;
        if ((c >= '0' && c <= '9') || c == '.') {
            errno = 0;
            char* end = nullptr;
            const double d = std::strtod(text.c_str() + pos, &end);
            if (end == text.c_str() + pos || errno == ERANGE || !std::isfinite(d))
                throw std::runtime_error("Invalid or non-finite numeric literal at " + std::to_string(pos));
            pos = static_cast<std::size_t>(end - text.c_str());
            out.push_back({Kind::Number, text.substr(start, pos - start), d, start});
            continue;
        }
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            do {
                const unsigned char next = static_cast<unsigned char>(text[pos]);
                if (!((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z') ||
                      (next >= '0' && next <= '9') || next == '_')) break;
                ++pos;
            } while (pos < text.size());
            std::string name = text.substr(start, pos - start);
            std::transform(name.begin(), name.end(), name.begin(),
                           [](unsigned char v) { return static_cast<char>(std::tolower(v)); });
            out.push_back({Kind::Name, name, 0.0, start});
            continue;
        }
        ++pos;
        Kind kind;
        switch (c) {
        case '+': kind = Kind::Plus; break;
        case '-': kind = Kind::Minus; break;
        case '*': kind = Kind::Mul; break;
        case '/': kind = Kind::Div; break;
        case '^': kind = Kind::Power; break;
        case '(': kind = Kind::Open; break;
        case ')': kind = Kind::Close; break;
        case ',': kind = Kind::Comma; break;
        default: throw std::runtime_error("Unsupported character at " + std::to_string(start));
        }
        out.push_back({kind, text.substr(start, 1), 0.0, start});
    }
    out.push_back({Kind::End, {}, 0.0, pos});
    return out;
}

[[nodiscard]] int builtinArity(const std::string& n) {
    if (n == "pow") return 2;
    for (const auto* candidate : {"sin", "cos", "tan", "asin", "acos", "atan",
                                  "sinh", "cosh", "tanh", "sqrt", "exp", "ln",
                                  "log", "log10", "abs", "arg", "conj", "real",
                                  "imag", "norm"}) {
        if (n == candidate) return 1;
    }
    return -1;
}

struct Node {
    enum class Type { Literal, Variable, Unary, Binary, Builtin, UserCall } type;
    ComplexValue literal = {0.0, 0.0};
    Kind op = Kind::End;
    std::string name;
    std::shared_ptr<const UserFunction> callable;
    std::vector<std::unique_ptr<Node>> arguments;
    explicit Node(Type t) : type(t) {}
};

class Parser {
public:
    Parser(const std::string& text, const VariableScope* names,
           const FunctionScope* funcs) : tokens_(lex(text)), variables_(names), functions_(funcs) {}
    [[nodiscard]] std::unique_ptr<Node> parse() {
        auto result = sum(0);
        if (peek().kind != Kind::End) fail("Unexpected token");
        return result;
    }
private:
    const Token& peek() const { return tokens_.at(cursor_); }
    bool take(Kind k) { if (peek().kind != k) return false; ++cursor_; return true; }
    [[noreturn]] void fail(const std::string& what) const {
        throw std::runtime_error(what + " at byte " + std::to_string(peek().location));
    }
    std::unique_ptr<Node> make(Node::Type type) {
        if (++nodeCount_ > maxNodes) fail("Expression exceeds node limit");
        return std::make_unique<Node>(type);
    }
    void guard(unsigned depth) const { if (depth > maxDepth) fail("Expression nesting too deep"); }
    std::unique_ptr<Node> binary(Kind op, std::unique_ptr<Node> left,
                                  std::unique_ptr<Node> right) {
        auto n = make(Node::Type::Binary);
        n->op = op;
        n->arguments.push_back(std::move(left));
        n->arguments.push_back(std::move(right));
        return n;
    }
    std::unique_ptr<Node> sum(unsigned depth) {
        guard(depth);
        auto lhs = product(depth + 1);
        while (peek().kind == Kind::Plus || peek().kind == Kind::Minus) {
            const Kind op = tokens_[cursor_++].kind;
            lhs = binary(op, std::move(lhs), product(depth + 1));
        }
        return lhs;
    }
    std::unique_ptr<Node> product(unsigned depth) {
        guard(depth);
        auto lhs = unary(depth + 1);
        for (;;) {
            Kind op = peek().kind;
            const bool implicit = (op == Kind::Name || op == Kind::Open);
            if (op != Kind::Mul && op != Kind::Div && !implicit) break;
            if (implicit) op = Kind::Mul; else ++cursor_;
            lhs = binary(op, std::move(lhs), unary(depth + 1));
        }
        return lhs;
    }
    std::unique_ptr<Node> unary(unsigned depth) {
        guard(depth);
        if (peek().kind == Kind::Plus || peek().kind == Kind::Minus) {
            const Kind op = tokens_[cursor_++].kind;
            auto n = make(Node::Type::Unary);
            n->op = op;
            n->arguments.push_back(unary(depth + 1));
            return n;
        }
        return power(depth + 1);
    }
    std::unique_ptr<Node> power(unsigned depth) {
        guard(depth);
        auto lhs = primary(depth + 1);
        if (take(Kind::Power)) lhs = binary(Kind::Power, std::move(lhs), unary(depth + 1));
        return lhs;
    }
    std::unique_ptr<Node> primary(unsigned depth) {
        guard(depth);
        if (peek().kind == Kind::Number) {
            auto result = make(Node::Type::Literal);
            result->literal = {tokens_[cursor_++].number, 0.0};
            return result;
        }
        if (peek().kind == Kind::Name) {
            const std::string name = tokens_[cursor_++].text;
            const int arity = builtinArity(name);
            auto callable = functions_ ? functions_->lookup(name) : nullptr;
            // Declared scalar variables and constants may be followed by an
            // implicit multiplication parenthesis, as in a(x+1) or pi(x).
            const bool ordinaryName = name == "i" || name == "z" ||
                name == "x" || name == "ans" || name == "pi" ||
                name == "tau" || name == "e" ||
                (variables_ && variables_->contains(name));
            if (peek().kind == Kind::Open && !ordinaryName &&
                (arity >= 0 || callable)) {
                take(Kind::Open);
                auto result = make(callable && arity < 0 ? Node::Type::UserCall : Node::Type::Builtin);
                result->name = name;
                result->callable = callable;
                if (!take(Kind::Close)) {
                    do { result->arguments.push_back(sum(depth + 1)); }
                    while (take(Kind::Comma));
                    if (!take(Kind::Close)) fail("Expected closing ')'");
                }
                const std::size_t expected = callable && arity < 0 ? callable->parameters.size() : static_cast<std::size_t>(arity);
                if (result->arguments.size() != expected) fail("Wrong number of arguments for '" + name + "'");
                return result;
            }
            if (name == "pi" || name == "tau" || name == "e" || name == "i") {
                auto result = make(Node::Type::Literal);
                result->literal = name == "i" ? ComplexValue{0.0, 1.0}
                    : ComplexValue{name == "pi" ? pi : name == "tau" ? 2*pi : std::numbers::e, 0.0};
                return result;
            }
            if (name == "z" || name == "x" || name == "ans" ||
                (variables_ && variables_->contains(name))) {
                auto result = make(Node::Type::Variable);
                result->name = name;
                return result;
            }
            fail("Unknown identifier '" + name + "'");
        }
        if (take(Kind::Open)) {
            auto expr = sum(depth + 1);
            if (!take(Kind::Close)) fail("Expected closing ')'");
            return expr;
        }
        fail("Expected expression");
    }
    std::vector<Token> tokens_;
    std::size_t cursor_ = 0, nodeCount_ = 0;
    const VariableScope* variables_;
    const FunctionScope* functions_;
};

[[nodiscard]] ComplexResult success(ComplexValue value) {
    if (!finiteComplex(value)) return {ComplexStatus::UndefinedValue, {}, "Non-finite complex result"};
    // Normalize signed zeros before branch-cut functions consume this value.
    // In particular, -1 from unary negation must select the principal +pi side.
    return {ComplexStatus::Success,
            {value.real() == 0.0 ? 0.0 : value.real(),
             value.imag() == 0.0 ? 0.0 : value.imag()}, {}};
}
[[nodiscard]] ComplexResult undefined(std::string message) {
    return {ComplexStatus::UndefinedValue, {}, std::move(message)};
}

struct Context {
    ComplexValue z, ans;
    AngleMode angle;
    const VariableBindings& bindings;
};

[[nodiscard]] ComplexResult apply(const std::string& name,
                                  const std::vector<ComplexValue>& a, AngleMode angle) {
    try {
        const ComplexValue v = a[0];
        const ComplexValue trig = angle == AngleMode::Degrees ? v * degToRad : v;
        const double inverseScale = angle == AngleMode::Degrees ? radToDeg : 1.0;
        if (name == "sin") return success(std::sin(trig));
        if (name == "cos") return success(std::cos(trig));
        if (name == "tan") return success(std::tan(trig));
        if (name == "asin") return success(std::asin(v) * inverseScale);
        if (name == "acos") return success(std::acos(v) * inverseScale);
        if (name == "atan") return success(std::atan(v) * inverseScale);
        if (name == "sinh") return success(std::sinh(v));
        if (name == "cosh") return success(std::cosh(v));
        if (name == "tanh") return success(std::tanh(v));
        if (name == "sqrt") return success(std::sqrt(v));
        if (name == "exp") return success(std::exp(v));
        if (name == "ln") return success(std::log(v));
        if (name == "log" || name == "log10") return success(std::log10(v));
        if (name == "abs") return success({std::abs(v), 0.0});
        if (name == "arg") {
            if (v == ComplexValue{0.0, 0.0}) return undefined("Argument of zero is undefined");
            return success({std::arg(v) * inverseScale, 0.0});
        }
        if (name == "conj") return success(std::conj(v));
        if (name == "real") return success({v.real(), 0.0});
        if (name == "imag") return success({v.imag(), 0.0});
        if (name == "norm") return success({std::norm(v), 0.0});
        if (name == "pow") {
            if (v == ComplexValue{0.0, 0.0} && a[1] == ComplexValue{0.0, 0.0})
                return undefined("Zero to the zeroth power is undefined");
            return success(std::pow(v, a[1]));
        }
    } catch (...) {
        return undefined("Complex mathematical function failed");
    }
    return {ComplexStatus::UnsupportedOperation, {}, "Unsupported function: " + name};
}

[[nodiscard]] ComplexResult eval(const Node& node, const Context& ctx,
                                  unsigned depth) {
    if (depth > maxDepth + 16) return undefined("Evaluation nesting exceeded");
    if (node.type == Node::Type::Literal) return success(node.literal);
    if (node.type == Node::Type::Variable) {
        if (node.name == "z") return success(ctx.z);
        if (node.name == "ans") return success(ctx.ans);
        if (node.name == "x") return success({ctx.z.real(), 0.0});
        double value = 0.0;
        if (!ctx.bindings.lookup(node.name, value))
            return undefined("Unbound real variable: " + node.name);
        return success({value, 0.0});
    }
    std::vector<ComplexValue> arguments;
    for (const auto& a : node.arguments) {
        auto result = eval(*a, ctx, depth + 1);
        if (!result.successful()) return result;
        arguments.push_back(result.value);
    }
    switch (node.type) {
    case Node::Type::Unary:
        return success(node.op == Kind::Minus ? -arguments[0] : arguments[0]);
    case Node::Type::Binary:
        switch (node.op) {
        case Kind::Plus: return success(arguments[0] + arguments[1]);
        case Kind::Minus: return success(arguments[0] - arguments[1]);
        case Kind::Mul: return success(arguments[0] * arguments[1]);
        case Kind::Div:
            if (arguments[1] == ComplexValue{0.0, 0.0}) return undefined("Division by zero");
            return success(arguments[0] / arguments[1]);
        case Kind::Power:
            if (arguments[0] == ComplexValue{0.0, 0.0} &&
                arguments[1] == ComplexValue{0.0, 0.0})
                return undefined("Zero to the zeroth power is undefined");
            return success(std::pow(arguments[0], arguments[1]));
        default: return undefined("Unknown binary operator");
        }
    case Node::Type::Builtin: return apply(node.name, arguments, ctx.angle);
    case Node::Type::UserCall: {
        if (!node.callable || !node.callable->body)
            return undefined("Missing real user-function definition");
        VariableBindings parameters;
        for (std::size_t j = 0; j < arguments.size(); ++j) {
            if (arguments[j].imag() != 0.0)
                return {ComplexStatus::UnsupportedOperation, {},
                        "Real user function cannot accept complex arguments"};
            if (!parameters.set(node.callable->parameters[j], arguments[j].real()))
                return undefined("Cannot bind real function parameter");
        }
        const double value = node.callable->body->evaluateWithParameters(
            ctx.z.real(), ctx.angle, ctx.bindings, parameters);
        return success({value, 0.0});
    }
    case Node::Type::Literal:
    case Node::Type::Variable: break;
    }
    return undefined("Unknown expression node");
}
} // namespace

struct ComplexExpression::Impl {
    std::unique_ptr<Node> root;
    std::string error;
};

ComplexExpression::ComplexExpression(const std::string& src)
    : impl_(std::make_unique<Impl>()) {
    try { impl_->root = Parser(src, nullptr, nullptr).parse(); }
    catch (const std::exception& e) { impl_->error = e.what(); }
}
ComplexExpression::ComplexExpression(const std::string& src, const VariableScope& vars)
    : impl_(std::make_unique<Impl>()) {
    try { impl_->root = Parser(src, &vars, nullptr).parse(); }
    catch (const std::exception& e) { impl_->error = e.what(); }
}
ComplexExpression::ComplexExpression(const std::string& src, const VariableScope& vars,
                                     const FunctionScope& functions)
    : impl_(std::make_unique<Impl>()) {
    try { impl_->root = Parser(src, &vars, &functions).parse(); }
    catch (const std::exception& e) { impl_->error = e.what(); }
}
ComplexExpression::~ComplexExpression() = default;
ComplexExpression::ComplexExpression(ComplexExpression&&) noexcept = default;
ComplexExpression& ComplexExpression::operator=(ComplexExpression&&) noexcept = default;
bool ComplexExpression::valid() const noexcept { return impl_ && impl_->root != nullptr; }
const std::string& ComplexExpression::error() const noexcept {
    static const std::string moved = "Moved-from complex expression";
    return impl_ ? impl_->error : moved;
}
ComplexResult ComplexExpression::evaluate(ComplexValue z, ComplexValue ans, AngleMode mode,
                                          const VariableBindings& bindings) const noexcept {
    if (!valid()) return {ComplexStatus::InvalidExpression, {}, error()};
    if (!finiteComplex(z) || !finiteComplex(ans))
        return {ComplexStatus::UndefinedValue, {}, "Non-finite input"};
    try { return eval(*impl_->root, Context{z, ans, mode, bindings}, 0); }
    catch (...) { return {ComplexStatus::UndefinedValue, {}, "Complex evaluation failed"}; }
}
ComplexResult evaluateComplex(const Expression& expr, double x, double ans,
                              AngleMode mode, const VariableBindings& bindings) noexcept {
    if (!expr.valid()) return {ComplexStatus::InvalidExpression, {}, expr.error()};
    const double value = expr.evaluate(x, ans, mode, bindings);
    return success({value, 0.0});
}
} // namespace rationalstudio
