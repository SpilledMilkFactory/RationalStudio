#pragma once

#include "Expression.hpp"

#include <complex>
#include <memory>
#include <string>
#include <string_view>

namespace rationalstudio {

// M4-04 / Issue #49: distinct complex-valued mathematical results.
// The established Expression::evaluate(double) and graphing API remain real-only.
using ComplexValue = std::complex<double>;

enum class ComplexStatus {
    Success,
    InvalidExpression,
    UndefinedValue,
    UnsupportedOperation
};

struct ComplexResult {
    ComplexStatus status = ComplexStatus::InvalidExpression;
    ComplexValue value = {0.0, 0.0};
    std::string message;
    [[nodiscard]] bool successful() const noexcept {
        return status == ComplexStatus::Success;
    }
};

[[nodiscard]] bool finiteComplex(ComplexValue value) noexcept;
[[nodiscard]] const char* complexStatusName(ComplexStatus status) noexcept;
// Canonical real, imaginary or a +/- bi form. Non-finite values print "undefined".
[[nodiscard]] std::string formatComplex(ComplexValue value, int precision = 12);

// A compiled, Qt-free complex expression, deliberately separate from the legacy
// scalar Expression AST. Complex literals use i, so "3+2i" is supported.
// z is an optional complex input, x is an optional real input, and ans is complex.
// Named VariableScope/VariableBindings values remain *real* scalar bindings.
// Reusable compiled real functions may be called when all arguments are real;
// complex-valued user-function definitions are not yet part of FunctionDefinitions.
class ComplexExpression {
public:
    explicit ComplexExpression(const std::string& source);
    ComplexExpression(const std::string& source, const VariableScope& variables);
    ComplexExpression(const std::string& source, const VariableScope& variables,
                      const FunctionScope& functions);
    ~ComplexExpression();
    ComplexExpression(ComplexExpression&&) noexcept;
    ComplexExpression& operator=(ComplexExpression&&) noexcept;
    ComplexExpression(const ComplexExpression&) = delete;
    ComplexExpression& operator=(const ComplexExpression&) = delete;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] const std::string& error() const noexcept;
    [[nodiscard]] ComplexResult evaluate(
        ComplexValue z = {0.0, 0.0}, ComplexValue ans = {0.0, 0.0},
        AngleMode angle = AngleMode::Radians,
        const VariableBindings& bindings = {}) const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Compatibility bridge: evaluate a legacy scalar compiled expression using the
// existing evaluator and promote its finite real result to the complex type.
// It never reinterprets a scalar grammar as a complex grammar.
[[nodiscard]] ComplexResult evaluateComplex(
    const Expression& expression, double x, double ans,
    AngleMode angle, const VariableBindings& bindings) noexcept;

} // namespace rationalstudio
