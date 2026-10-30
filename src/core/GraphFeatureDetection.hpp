#pragma once

#include "Expression.hpp"

#include <cstddef>
#include <functional>
#include <vector>

namespace rationalstudio {

// M3-05 / Issue #41: bounded, heuristic analysis of explicit Cartesian curves.
// Points are numerical candidates, not certified complete or differentiable.
using GraphFeatureFunction = std::function<double(double)>;

enum class GraphFeatureKind { Root, LocalMinimum, LocalMaximum, StationaryInflection };
enum class GraphFeatureStatus {
    Complete, InvalidInput, EvaluationLimit, ResultLimit, CoincidentWithAxis // includes sampled zero intervals
};

struct GraphFeatureOptions {
    std::size_t subdivisions = 512;
    std::size_t maxEvaluations = 30000; // actual function callback invocations
    std::size_t maxResults = 48;
    double xTolerance = 1e-8;
    double residualTolerance = 1e-8;   // |f(x)| for roots
    double derivativeTolerance = 1e-6; // |df/dx| for stationary inflections
};

struct GraphFeaturePoint {
    GraphFeatureKind kind = GraphFeatureKind::Root;
    double x = 0;
    double y = 0;
    double residual = 0;       // roots: |f(x)|; extrema: local margin; inflections: |df/dx|
    double xBracketWidth = 0;  // numerical search interval, not an error guarantee
};

struct GraphFeatureResult {
    GraphFeatureStatus status = GraphFeatureStatus::InvalidInput;
    std::vector<GraphFeaturePoint> points;
    std::size_t evaluations = 0;
    std::size_t rejectedCandidates = 0;
    [[nodiscard]] bool complete() const noexcept { return status == GraphFeatureStatus::Complete; }
};

// Only supported explicit Cartesian f(x); endpoints can be roots but are not
// classified as extrema. Searches can miss sub-grid features and singularities.
[[nodiscard]] GraphFeatureResult inspectCartesianFeatures(
    const GraphFeatureFunction& function, double minimum, double maximum,
    const GraphFeatureOptions& options = {});

[[nodiscard]] GraphFeatureResult inspectCartesianFeatures(
    const Expression& expression, double minimum, double maximum, AngleMode angle,
    const VariableBindings& bindings, double ans = 0.0,
    const GraphFeatureOptions& options = {});

} // namespace rationalstudio
