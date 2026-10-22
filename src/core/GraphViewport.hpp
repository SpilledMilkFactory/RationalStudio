#pragma once

namespace rationalstudio {

    // Mathematical state for a Cartesian viewport. This type deliberately has
    // no Qt dependencies; all screen coordinates are expressed as doubles.
    //
    // World coordinates: +X right, +Y up.
    // Screen coordinates: +X right, +Y down, origin at the top-left.
    class GraphViewport final {
    public:
        static constexpr double kMinScale = 0.1;
        static constexpr double kMaxScale = 20000.0;

        // Preserve the zoom constraints of the existing mouse-wheel interaction.
        static constexpr double kMinWheelScale = 0.5;
        static constexpr double kMaxWheelScale = 5000.0;

        [[nodiscard]] bool setDimensions(int width, int height) noexcept;

        [[nodiscard]] bool setView(
            double centerX,
            double centerY,
            double pixelsPerUnit
        ) noexcept;

        [[nodiscard]] bool panPixels(
            double deltaX,
            double deltaY
        ) noexcept;

        [[nodiscard]] bool zoomAt(
            double factor,
            double screenX,
            double screenY
        ) noexcept;

        [[nodiscard]] double worldToScreenX(double x) const noexcept;
        [[nodiscard]] double worldToScreenY(double y) const noexcept;

        [[nodiscard]] double screenToWorldX(double x) const noexcept;
        [[nodiscard]] double screenToWorldY(double y) const noexcept;

        [[nodiscard]] bool isValid() const noexcept;

        [[nodiscard]] int width() const noexcept {
            return width_;
        }

        [[nodiscard]] int height() const noexcept {
            return height_;
        }

        [[nodiscard]] double centerX() const noexcept {
            return centerX_;
        }

        [[nodiscard]] double centerY() const noexcept {
            return centerY_;
        }

        [[nodiscard]] double pixelsPerUnit() const noexcept {
            return pixelsPerUnit_;
        }

    private:
        // A default-constructed viewport is valid before a widget receives its
        // first resize event. Dimensions are replaced with actual widget size.
        int width_ = 1;
        int height_ = 1;

        double centerX_ = 0.0;
        double centerY_ = 0.0;
        double pixelsPerUnit_ = 58.0;
    };

} // namespace rationalstudio
