#include "GraphViewport.hpp"

#include <algorithm>
#include <cmath>

namespace rationalstudio {

    bool GraphViewport::isValid() const noexcept {
        return width_ > 0 &&
        height_ > 0 &&
        std::isfinite(centerX_) &&
        std::isfinite(centerY_) &&
        std::isfinite(pixelsPerUnit_) &&
        pixelsPerUnit_ >= kMinScale &&
        pixelsPerUnit_ <= kMaxScale;
    }

    bool GraphViewport::setDimensions(
        int width,
        int height
    ) noexcept {
        if (width <= 0 || height <= 0) {
            return false;
        }

        width_ = width;
        height_ = height;

        return true;
    }

    bool GraphViewport::setView(
        double centerX,
        double centerY,
        double pixelsPerUnit
    ) noexcept {
        if (!std::isfinite(centerX) ||
            !std::isfinite(centerY) ||
            !std::isfinite(pixelsPerUnit) ||
            pixelsPerUnit < kMinScale ||
            pixelsPerUnit > kMaxScale) {
            return false;
            }

            centerX_ = centerX;
        centerY_ = centerY;
        pixelsPerUnit_ = pixelsPerUnit;

        return true;
    }

    double GraphViewport::worldToScreenX(
        double x
    ) const noexcept {
        return std::fma(
            x - centerX_,
            pixelsPerUnit_,
            width_ * 0.5
        );
    }

    double GraphViewport::worldToScreenY(
        double y
    ) const noexcept {
        return std::fma(
            centerY_ - y,
            pixelsPerUnit_,
            height_ * 0.5
        );
    }

    double GraphViewport::screenToWorldX(
        double x
    ) const noexcept {
        return std::fma(
            x - width_ * 0.5,
            1.0 / pixelsPerUnit_,
            centerX_
        );
    }

    double GraphViewport::screenToWorldY(
        double y
    ) const noexcept {
        return std::fma(
            height_ * 0.5 - y,
            1.0 / pixelsPerUnit_,
            centerY_
        );
    }

    bool GraphViewport::panPixels(
        double deltaX,
        double deltaY
    ) noexcept {
        if (!isValid() ||
            !std::isfinite(deltaX) ||
            !std::isfinite(deltaY)) {
            return false;
            }

            // Dragging right moves the visible plot right;
            // dragging down moves it down.
            return setView(
                centerX_ - deltaX / pixelsPerUnit_,
                centerY_ + deltaY / pixelsPerUnit_,
                pixelsPerUnit_
            );
    }

    bool GraphViewport::zoomAt(
        double factor,
        double screenX,
        double screenY
    ) noexcept {
        if (!isValid() ||
            !std::isfinite(factor) ||
            factor <= 0.0 ||
            !std::isfinite(screenX) ||
            !std::isfinite(screenY)) {
            return false;
            }

            // Compute the new scale without overflowing
            // when factor is enormous.
            double newScale = pixelsPerUnit_ * factor;

        if (!std::isfinite(newScale)) {
            newScale = kMaxWheelScale;
        }

        newScale = std::clamp(
            newScale,
            kMinWheelScale,
            kMaxWheelScale
        );

        // The world point underneath the pointer must
        // remain under that pixel.
        const double worldX = screenToWorldX(screenX);
        const double worldY = screenToWorldY(screenY);

        const double newCenterX =
        worldX - (screenX - width_ * 0.5) / newScale;

        const double newCenterY =
        worldY + (screenY - height_ * 0.5) / newScale;

        // setView validates all values before mutating the viewport.
        return setView(
            newCenterX,
            newCenterY,
            newScale
        );
    }

} // namespace rationalstudio
