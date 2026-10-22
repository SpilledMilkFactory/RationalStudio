#pragma once

#include "GraphViewport.hpp"

#include <cstddef>
#include <functional>
#include <vector>

namespace rationalstudio {

    // Screen-space drawing vertex for an explicit Cartesian y = f(x) curve.
    // A moveTo vertex begins a new stroke; other vertices extend that stroke.
    // The sampler has no dependency on Qt or on the expression parser.
    struct CurveVertex {
        double screenX = 0.0;
        double screenY = 0.0;
        bool moveTo = false;
    };

    struct CurveSamplingOptions {
        double coarseStepPixels = 24.0;
        double tolerancePixels = 0.75;
        double minStepPixels = 0.375;
        unsigned maxDepth = 9;

        // A per-call safety ceiling; a comprehensive, cross-curve sampling
        // budget and scheduling policy are tracked separately in issue #21.
        std::size_t maxEvaluations = 65536;
    };

    struct CurveSamples {
        std::vector<CurveVertex> vertices;
        std::size_t evaluations = 0;
        bool truncated = false;
    };

    // Adaptively approximate y=f(x) over the current screen-width interval.
    // The callback receives world-space x; the result is screen-space geometry.
    // Assumes neither continuity nor mathematical domain knowledge. Detection
    // of all discontinuities and high-frequency detail is not guaranteed.
    [[nodiscard]] CurveSamples sampleExplicitCurve(
        const GraphViewport& viewport,
        const std::function<double(double)>& function,
                                                   CurveSamplingOptions options = {}
    );

} // namespace rationalstudio
