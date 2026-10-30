#pragma once

#include "Expression.hpp"

#include <cstddef>
#include <functional>
#include <vector>

namespace rationalstudio {

// Issue #40 — bounded numerical intersections of explicit Cartesian functions.
// The routine finds *isolated* zeros of f(x)-g(x), including sampled tangencies.
// It cannot certify that every root in a finite domain has been found.
using CartesianIntersectionFunction = std::function<double(double)>;

enum class IntersectionStatus {
    Complete,          // Scan finished, with no detected unresolved resource limit.
    InvalidInput,      // Nonfinite/reversed range or invalid numeric limits.
    Coincident,        // Sampled functions agree throughout the finite scan.
    EvaluationLimit,   // Budget exhausted. Results are incomplete.
    ResultLimit        // Result cap reached. Results are incomplete.
};

struct IntersectionOptions {
    std::size_t subdivisions = 512;
    std::size_t maxEvaluations = 65536; // counts calls to BOTH function callbacks
    std::size_t maxResults = 64;
    double xTolerance = 1e-8;
    double residualTolerance = 1e-8;
};

struct IntersectionPoint {
    double x = 0.0;
    double y = 0.0;
    double residual = 0.0;          // |f(x)-g(x)| in world Y units
    double xBracketWidth = 0.0;     // last refinement interval (not a guarantee)
};

struct IntersectionResult {
    IntersectionStatus status = IntersectionStatus::InvalidInput;
    std::vector<IntersectionPoint> points;
    std::size_t evaluations = 0;
    std::size_t rejectedCandidates = 0;
    [[nodiscard]] bool complete() const noexcept { return status == IntersectionStatus::Complete; }
};

// The callbacks are invoked synchronously; no references or state are retained.
// Undefined/nonfinite values are excluded and do not certify intersections.
[[nodiscard]] IntersectionResult findCartesianIntersections(
    const CartesianIntersectionFunction& first,
    const CartesianIntersectionFunction& second,
    double minimum, double maximum,
    const IntersectionOptions& options = {});

// Evaluate two existing compiled expressions against one immutable binding
// snapshot; function/slider definitions do not need recompilation.
[[nodiscard]] IntersectionResult findCartesianIntersections(
    const Expression& first, const Expression& second,
    double minimum, double maximum, AngleMode angle,
    const VariableBindings& bindings, double ans = 0.0,
    const IntersectionOptions& options = {});

} // namespace rationalstudio
