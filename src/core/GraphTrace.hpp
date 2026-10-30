#pragma once

#include "CurveSampler.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace rationalstudio {

// A trace is based on already-completed screen-space geometry. It cannot
// bridge moveTo discontinuities or invent unsampled contour topology.
// Any parameter is interpolated from genuine parametric sample endpoints.
struct GraphTraceHit {
    double screenX = 0.0;
    double screenY = 0.0;
    double distancePixels = 0.0;
    double parameter = 0.0;
    bool hasParameter = false;
    std::size_t segment = 0;
};

[[nodiscard]] std::optional<GraphTraceHit> nearestGraphTrace(
    const std::vector<CurveVertex>& vertices,
    double cursorX, double cursorY, double radiusPixels = 20.0,
    std::size_t maxVertices = 131072) noexcept;

} // namespace rationalstudio
