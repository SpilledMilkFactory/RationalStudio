#pragma once

#include <cmath>
#include <algorithm>
#include <limits>
#include <optional>

namespace rationalstudio {

// An axis in pixel coordinates measured from the lower world-space edge:
// X: pixels from the left; Y: pixels from the bottom. Consumers must
// clip the resulting screen positions to the widget bounds.
struct GraphGridAxis {
    double firstPixel = 0.0;
    double spacingPixels = 0.0;
    double firstWorld = 0.0;
    double spacingWorld = 0.0;
};

// Compute grid positions without integer-casting enormous projected world
// coordinates. When the viewport lacks enough floating-point precision to
// resolve a subpixel grid phase, skip that grid instead of drawing repeated
// or misplaced lines. This is presentation-only and does not alter the view.
[[nodiscard]] inline std::optional<GraphGridAxis> graphGridAxis(
    double center, int pixelExtent, double pixelsPerUnit) noexcept {
    if (pixelExtent <= 0 || !std::isfinite(center) ||
        !std::isfinite(pixelsPerUnit) || pixelsPerUnit <= 0.0) {
        return std::nullopt;
    }

    const double unitsPerPixel = 1.0 / pixelsPerUnit;
    const double targetStep = 85.0 * unitsPerPixel;
    if (!std::isfinite(targetStep) || targetStep <= 0.0) return std::nullopt;

    const double power = std::pow(10.0, std::floor(std::log10(targetStep)));
    if (!std::isfinite(power) || power <= 0.0) return std::nullopt;
    const double fraction = targetStep / power;
    const double step = (fraction <= 1.0 ? 1.0 :
                         fraction <= 2.0 ? 2.0 :
                         fraction <= 5.0 ? 5.0 : 10.0) * power;
    const double pixelStep = step * pixelsPerUnit;
    if (!std::isfinite(step) || step <= 0.0 ||
        !std::isfinite(pixelStep) || pixelStep < 4.0) return std::nullopt;

    // setView intentionally accepts huge finite centers for compatibility.
    // At such centers, neighboring doubles can already span multiple screen
    // pixels. Grid phase and text positions cannot be represented reliably.
    const double successor = std::nextafter(center,
        std::numeric_limits<double>::infinity());
    const double predecessor = std::nextafter(center,
        -std::numeric_limits<double>::infinity());
    const double ulp = std::max(successor - center, center - predecessor);
    if (!std::isfinite(ulp) || ulp * pixelsPerUnit > 0.25)
        return std::nullopt;

    const double edge = std::fma(-static_cast<double>(pixelExtent) * 0.5,
                                 unitsPerPixel, center);
    if (!std::isfinite(edge)) return std::nullopt;
    const double remainder = std::fmod(edge, step);
    if (!std::isfinite(remainder)) return std::nullopt;
    const double offset = remainder <= 0.0 ? -remainder : step - remainder;
    const double firstPixel = offset * pixelsPerUnit;
    const double firstWorld = edge + offset;
    if (!std::isfinite(firstPixel) || !std::isfinite(firstWorld))
        return std::nullopt;
    return GraphGridAxis{firstPixel, pixelStep, firstWorld, step};
}

} // namespace rationalstudio
