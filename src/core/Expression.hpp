#pragma once
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

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

// Compiled expressions are immutable. Legacy construction rejects unknown
// identifiers; opt-in construction accepts only names declared in the scope.
// Resolution happens on each evaluation without modifying the syntax tree.
class Expression {
public:
    explicit Expression(const std::string& source);
    Expression(const std::string& source, const VariableScope& scope);
    ~Expression();
    Expression(Expression&&) noexcept;
    Expression& operator=(Expression&&) noexcept;
    Expression(const Expression&) = delete;
    Expression& operator=(const Expression&) = delete;

    bool valid() const noexcept;
    const std::string& error() const noexcept;
    bool dependsOnX() const noexcept;
    bool dependsOnAns() const noexcept;
    const std::vector<std::string>& referencedVariables() const noexcept;
    double evaluate(double x = 0, double ans = 0,
                    AngleMode angle = AngleMode::Radians) const noexcept;
    double evaluate(double x, double ans, AngleMode angle,
                    const VariableBindings& bindings) const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    double evaluateImpl(double x, double ans, AngleMode angle,
                        const VariableBindings* bindings) const noexcept;
};

} // namespace rationalstudio
