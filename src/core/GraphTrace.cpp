#include "GraphTrace.hpp"

#include <algorithm>
#include <cmath>

namespace rationalstudio {
std::optional<GraphTraceHit> nearestGraphTrace(
    const std::vector<CurveVertex>& vertices,
    double cursorX, double cursorY, double radiusPixels,
    std::size_t maxVertices) noexcept {
    if (!std::isfinite(cursorX) || !std::isfinite(cursorY) ||
        !std::isfinite(radiusPixels) || radiusPixels <= 0 ||
        maxVertices == 0 || vertices.empty()) return std::nullopt;
    double best = radiusPixels * radiusPixels;
    if (!std::isfinite(best)) return std::nullopt;
    std::optional<GraphTraceHit> hit;
    const std::size_t count = std::min(vertices.size(), maxVertices);
    for (std::size_t i = 0; i < count; ++i) {
        const auto& b = vertices[i];
        if (!std::isfinite(b.screenX) || !std::isfinite(b.screenY)) continue;
        double x = b.screenX, y = b.screenY, alpha = 1.0;
        bool segment = i != 0 && !b.moveTo;
        const CurveVertex* a = segment ? &vertices[i - 1] : nullptr;
        if (a && std::isfinite(a->screenX) && std::isfinite(a->screenY)) {
            const double dx = b.screenX - a->screenX;
            const double dy = b.screenY - a->screenY;
            const double length2 = dx * dx + dy * dy;
            if (std::isfinite(length2) && length2 > 0) {
                alpha = std::clamp(((cursorX - a->screenX) * dx +
                                    (cursorY - a->screenY) * dy) / length2,
                                   0.0, 1.0);
                x = a->screenX + alpha * dx;
                y = a->screenY + alpha * dy;
            }
        } else { segment = false; }
        const double dx = cursorX - x, dy = cursorY - y;
        // Reject distant candidates before squaring to avoid overflow.
        if (std::abs(dx) > radiusPixels || std::abs(dy) > radiusPixels) continue;
        const double d2 = dx * dx + dy * dy;
        if (!std::isfinite(d2) || d2 >= best) continue;
        best = d2;
        GraphTraceHit found;
        found.screenX = x;
        found.screenY = y;
        found.distancePixels = std::sqrt(d2);
        found.segment = i;
        if (std::isfinite(b.parameter) &&
            (!segment || (a && std::isfinite(a->parameter)))) {
            found.parameter = segment ? std::lerp(a->parameter, b.parameter, alpha)
                                      : b.parameter;
            found.hasParameter = std::isfinite(found.parameter);
        }
        hit = found;
    }
    return hit;
}
} // namespace rationalstudio
