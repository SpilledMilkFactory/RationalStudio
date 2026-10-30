#pragma once

#include "Expression.hpp"

#include <cstddef>
#include <functional>
#include <limits>

namespace rationalstudio {

// M4-01 / Issue #46: pointwise finite-difference differentiation.
// A successful result is an estimate, not a mathematical differentiability proof.
using DifferentiationFunction = std::function<double(double)>;

enum class DerivativeMethod { None, CentralRichardson, ForwardRichardson, BackwardRichardson };
enum class DerivativeStatus {
    Converged,
    InvalidInput,
    UndefinedAtPoint,
    EvaluationLimit,
    NoFiniteStencil,
    NonDifferentiable,
    Unstable
};

struct DifferentiationOptions {
    double initialRelativeStep = 0.01;
    double minimumRelativeStep = 1e-12;
    double absoluteTolerance = 1e-8;
    double relativeTolerance = 1e-6;
    // Controls agreement of left/right secant slopes at a central candidate.
    // It is a heuristic smoothness guard, NOT a mathematical proof.
    double sideAgreementTolerance = 1e-3;
    std::size_t maxRefinements = 16;
    std::size_t maxEvaluations = 128;
    bool allowOneSided = true;
};

struct DerivativeResult {
    DerivativeStatus status = DerivativeStatus::InvalidInput;
    DerivativeMethod method = DerivativeMethod::None;
    double value = std::numeric_limits<double>::quiet_NaN();
    double estimatedError = std::numeric_limits<double>::infinity();
    double step = 0;
    std::size_t evaluations = 0; // number of callback invocations (including nonfinite)
    std::size_t refinements = 0;
    [[nodiscard]] bool converged() const noexcept { return status == DerivativeStatus::Converged; }
};

// Evaluate f'(x) using central O(h^2) secants and Richardson extrapolation.
// When a whole side is outside the domain, optional three-point O(h^2)
// forward/backward stencils can be used. Does not differentiate through a
// discontinuity, find symbolic derivatives, or certify differentiability.
[[nodiscard]] DerivativeResult differentiate(
    const DifferentiationFunction& function, double x,
    const DifferentiationOptions& options = {});

// Derivative with respect to the explicit Cartesian x variable. Other scalar
// names and captured user functions use immutable per-call binding snapshots;
// ans and the supplied RAD/DEG mode follow ordinary Expression semantics.
[[nodiscard]] DerivativeResult differentiate(
    const Expression& expression, double x, AngleMode angle,
    const VariableBindings& bindings, double ans = 0.0,
    const DifferentiationOptions& options = {});

[[nodiscard]] const char* derivativeStatusName(DerivativeStatus status) noexcept;
[[nodiscard]] const char* derivativeMethodName(DerivativeMethod method) noexcept;

} // namespace rationalstudio
