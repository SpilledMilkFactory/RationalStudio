#pragma once

#include "Expression.hpp"

#include <cstddef>
#include <functional>
#include <limits>

namespace rationalstudio {

    // M4-02 / Issue #47: adaptive finite-interval scalar quadrature.
    // A successful result is a numerical approximation, NOT an error certificate.
    using IntegrationFunction = std::function<double(double)>;

    enum class IntegrationMethod { GaussKronrod15 };
    enum class IntegrationStatus {
        Converged,
        InvalidInput,
        UndefinedIntegrand,
        EvaluationLimit,
        SubdivisionLimit,
        Unresolved
    };

    struct IntegrationOptions {
        double absoluteTolerance = 1e-9;
        double relativeTolerance = 1e-8;
        std::size_t maxEvaluations = 100000;
        std::size_t maxSubdivisions = 4096;
        std::size_t maxDepth = 80;
    };

    struct IntegrationResult {
        IntegrationStatus status = IntegrationStatus::InvalidInput;
        IntegrationMethod method = IntegrationMethod::GaussKronrod15;
        // Only meaningful as a published integral if converged() is true.
        double value = std::numeric_limits<double>::quiet_NaN();
        double estimatedError = std::numeric_limits<double>::infinity();
        std::size_t evaluations = 0;   // actual callback invocations, including invalid
        std::size_t subdivisions = 0;  // successful parent -> two children splits
        std::size_t intervals = 0;     // active intervals in the adaptive partition
        [[nodiscard]] bool converged() const noexcept {
            return status == IntegrationStatus::Converged;
        }
    };

    // Finite bounds only; reversed bounds negate the result, equal bounds integrate
    // to exactly zero without evaluating f. The sampled integrand must be finite.
    // Non-finite / throwing evaluations fail explicitly rather than being skipped.
    // Embedded Gauss(7)/Kronrod(15) error estimates guide bisection but cannot
    // prove convergence for arbitrary discontinuous/oscillatory/singular functions.
    [[nodiscard]] IntegrationResult integrate(
        const IntegrationFunction& function, double lower, double upper,
        const IntegrationOptions& options = {});

    // Reuses immutable compiled-expression semantics (x, ans, angle mode, bindings,
    // and compiled captured user-functions) without adding expression grammar.
    [[nodiscard]] IntegrationResult integrate(
        const Expression& expression, double lower, double upper,
        AngleMode angle, const VariableBindings& bindings, double ans = 0.0,
        const IntegrationOptions& options = {});

    [[nodiscard]] const char* integrationStatusName(IntegrationStatus status) noexcept;
    [[nodiscard]] const char* integrationMethodName(IntegrationMethod method) noexcept;

} // namespace rationalstudio
