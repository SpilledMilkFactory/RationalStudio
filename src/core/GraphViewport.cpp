#include "GraphViewport.hpp"

#include <algorithm>
#include <cmath>

namespace rationalstudio {

bool GraphViewport::isValid() const noexcept {
    return width_ > 0 && height_ > 0 &&
           std::isfinite(centerX_) && std::isfinite(centerY_) &&
           std::isfinite(pixelsPerUnit_) &&
           pixelsPerUnit_ >= kMinScale && pixelsPerUnit_ <= kMaxScale;
}

bool GraphViewport::setDimensions(int width, int height) noexcept {
    if (width <= 0 || height <= 0) return false;
    width_ = width;
    height_ = height;
    return true;
}

bool GraphViewport::setView(double centerX, double centerY,
                            double pixelsPerUnit) noexcept {
    if (!std::isfinite(centerX) || !std::isfinite(centerY) ||
        !std::isfinite(pixelsPerUnit) ||
        pixelsPerUnit < kMinScale || pixelsPerUnit > kMaxScale) {
        return false;
    }
    centerX_ = centerX;
    centerY_ = centerY;
    pixelsPerUnit_ = pixelsPerUnit;
    return true;
}

double GraphViewport::worldToScreenX(double x) const noexcept {
    return std::fma(x - centerX_, pixelsPerUnit_, width_ * 0.5);
}

double GraphViewport::worldToScreenY(double y) const noexcept {
    return std::fma(centerY_ - y, pixelsPerUnit_, height_ * 0.5);
}

double GraphViewport::screenToWorldX(double x) const noexcept {
    return std::fma(x - width_ * 0.5, 1.0 / pixelsPerUnit_, centerX_);
}

double GraphViewport::screenToWorldY(double y) const noexcept {
    return std::fma(height_ * 0.5 - y, 1.0 / pixelsPerUnit_, centerY_);
}

bool GraphViewport::panPixels(double deltaX, double deltaY) noexcept {
    if (!isValid() || !std::isfinite(deltaX) || !std::isfinite(deltaY))
        return false;
    if (deltaX == 0.0 && deltaY == 0.0) return true;

    const double reciprocalScale = 1.0 / pixelsPerUnit_;
    // Fused updates avoid a separate rounding of the displacement.
    const double nextX = std::fma(-deltaX, reciprocalScale, centerX_);
    const double nextY = std::fma( deltaY, reciprocalScale, centerY_);

    // Compare requested drag with the movement that double precision can
    // actually represent. Refuse visibly wrong (>1/4 pixel) navigation,
    // while allowing imperceptibly small subpixel no-ops.
    if (!std::isfinite(nextX) || !std::isfinite(nextY)) return false;
    const double errorX = std::fma(centerX_ - nextX, pixelsPerUnit_, -deltaX);
    const double errorY = std::fma(nextY - centerY_, pixelsPerUnit_, -deltaY);
    if (std::abs(errorX) > 0.25 || std::abs(errorY) > 0.25) return false;
    return setView(nextX, nextY, pixelsPerUnit_);
}

bool GraphViewport::zoomAt(double factor, double screenX,
                           double screenY) noexcept {
    if (!isValid() || !std::isfinite(factor) || factor <= 0.0 ||
        !std::isfinite(screenX) || !std::isfinite(screenY)) {
        return false;
    }
    if (factor == 1.0) return true;

    // Programmatic setView permits a wider range than the wheel. If a
    // restored view starts outside wheel limits, don't make wheel-in zoom
    // OUT or wheel-out zoom IN. Approach the wheel range monotonically.
    const double lower = std::min(kMinWheelScale, pixelsPerUnit_);
    const double upper = std::max(kMaxWheelScale, pixelsPerUnit_);
    const double nextScale = std::clamp(pixelsPerUnit_ * factor, lower, upper);

    // At a zoom boundary, preserve the center exactly (no cursor drift).
    if (nextScale == pixelsPerUnit_) return true;

    // Algebraically: newCenter = oldCenter + cursorOffset *
    // (1/oldScale - 1/newScale). Avoid computing an intermediate world
    // coordinate by adding a tiny cursor offset to a large center first.
    const double reciprocalDifference =
        (1.0 / pixelsPerUnit_) - (1.0 / nextScale);
    const double nextX = std::fma(screenX - width_ * 0.5,
                                  reciprocalDifference, centerX_);
    const double nextY = std::fma(height_ * 0.5 - screenY,
                                  reciprocalDifference, centerY_);

    // Estimate remaining cursor drift in screen pixels after rounding the
    // next center to a representable double. Reject only visible (>1/4 px)
    // errors; tiny anchors need not move a large center by a full ULP.
    if (!std::isfinite(nextX) || !std::isfinite(nextY)) return false;
    const double ratioMinusOne = nextScale / pixelsPerUnit_ - 1.0;
    const double driftX = std::fma(centerX_ - nextX, nextScale,
                                   (screenX - width_ * 0.5) * ratioMinusOne);
    const double driftY = std::fma(nextY - centerY_, nextScale,
                                   (height_ * 0.5 - screenY) * -ratioMinusOne);
    if (std::abs(driftX) > 0.25 || std::abs(driftY) > 0.25) return false;
    return setView(nextX, nextY, nextScale);
}

} // namespace rationalstudio
