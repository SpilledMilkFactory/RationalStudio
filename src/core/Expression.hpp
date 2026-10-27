#pragma once
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <cstddef>
#include <optional>

namespace rationalstudio {

enum class AngleMode { Radians, Degrees };

// Names are ASCII identifiers [A-Za-z_][A-Za-z0-9_]*, max 64 bytes,
// normalized to lowercase. Builtin names, x, ans and constants are reserved.
// This scope only declares which names may appear in an expression; it does
// not assign values. Assignment and dependency tracking are separate M2 tasks.
class VariableScope {
public:
    bool declare(std::string_view name, std::string* error = nullptr);
    bool contains(std::string_view name) const;
    const std::set<std::string>& names() const noexcept { return names_; }
private:
    std::set<std::string> names_;
};

// Call-scoped values: no reference to this object is retained by Expression.
// Missing or non-finite values produce an undefined (NaN) expression result.
class VariableBindings {
public:
    bool set(std::string_view name, double value);
    bool erase(std::string_view name);
    bool lookup(std::string_view name, double& value) const;
private:
    std::map<std::string, double, std::less<>> values_;
};

// Positions use zero-based byte offsets in the source passed to Expression.
// When the GUI normalizes Unicode symbols, it maps these offsets back to
// visible character columns via MathExpressionInput::sourceColumn().
enum class ExpressionErrorKind {
    Lexical, Syntax, UnknownIdentifier, FunctionCall, Arity, Limit
};

struct ExpressionDiagnostic {
    ExpressionErrorKind kind = ExpressionErrorKind::Syntax;
    std::string message;            // Human-readable, without duplicated position suffix.
    std::size_t offset = 0;         // Byte position in parser-compatible source.
    std::size_t length = 0;         // Zero means expected input at end of expression.
};

class Expression;

// Immutable compiled user-function definition. Calls retain a shared snapshot;
// redefinitions publish a new scope rather than modifying an existing AST.
struct UserFunction {
    std::string name;
    std::vector<std::string> parameters;
    std::shared_ptr<const Expression> body;
    std::vector<std::string> variableDependencies;
};

class FunctionScope {
public:
    [[nodiscard]] bool contains(std::string_view name) const;
    [[nodiscard]] std::shared_ptr<const UserFunction> lookup(std::string_view name) const;
    [[nodiscard]] std::vector<std::string> names() const;
private:
    std::map<std::string, std::shared_ptr<const UserFunction>, std::less<>> entries_;
    friend class FunctionDefinitions;
};

// Compiled expressions are immutable. Legacy construction rejects unknown
// identifiers; opt-in construction accepts only names declared in the scope.
// Resolution happens on each evaluation without modifying the syntax tree.
class Expression {
public:
    explicit Expression(const std::string& source);
    Expression(const std::string& source, const VariableScope& scope);
    Expression(const std::string& source, const VariableScope& scope,
               const FunctionScope& functions);
    ~Expression();
    Expression(Expression&&) noexcept;
    Expression& operator=(Expression&&) noexcept;
    Expression(const Expression&) = delete;
    Expression& operator=(const Expression&) = delete;

    bool valid() const noexcept;
    const std::string& error() const noexcept;
    // nullptr if valid; structured error details for inline editor feedback.
    const ExpressionDiagnostic* diagnostic() const noexcept;
    bool dependsOnX() const noexcept;
    bool dependsOnAns() const noexcept;
    const std::vector<std::string>& referencedVariables() const noexcept;
    const std::vector<std::string>& referencedFunctions() const noexcept;
    // Ambient scalar dependencies propagated through called functions.
    const std::vector<std::string>& functionVariableDependencies() const noexcept;
    double evaluate(double x = 0, double ans = 0,
                    AngleMode angle = AngleMode::Radians) const noexcept;
    double evaluate(double x, double ans, AngleMode angle,
                    const VariableBindings& bindings) const noexcept;
    // Internal function-call API: lexical parameters are distinct from
    // ambient variables, preventing the caller's parameters from leaking into
    // the called function's captured scalar names.
    double evaluateWithParameters(double x, AngleMode angle,
                                  const VariableBindings& ambient,
                                  const VariableBindings& parameters) const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    double evaluateImpl(double x, double ans, AngleMode angle,
                        const VariableBindings* bindings,
                        const VariableBindings* local = nullptr) const noexcept;
};

} // namespace rationalstudio
