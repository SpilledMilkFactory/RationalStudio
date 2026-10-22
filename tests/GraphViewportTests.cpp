#include "GraphViewport.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using rationalstudio::GraphViewport;

namespace {

    int checks = 0;

    void check(
        bool condition,
        const std::string& message
    ) {
        if (!condition) {
            throw std::runtime_error(message);
        }

        ++checks;
    }

    void near(
        double actual,
        double expected,
        double tolerance,
        const std::string& message
    ) {
        check(
            std::isfinite(actual) &&
            std::abs(actual - expected) <= tolerance,
              message
        );
    }

    void sameState(
        const GraphViewport& a,
        const GraphViewport& b,
        const std::string& message
    ) {
        check(
            a.width() == b.width() &&
            a.height() == b.height() &&
            a.centerX() == b.centerX() &&
            a.centerY() == b.centerY() &&
            a.pixelsPerUnit() == b.pixelsPerUnit(),
              message
        );
    }

} // namespace

int main() {
    try {
        GraphViewport view;

        check(
            view.isValid(),
              "default viewport must be valid"
        );

        check(
            view.width() == 1 && view.height() == 1,
              "default viewport dimensions"
        );

        check(
            view.setDimensions(800, 600),
              "set valid dimensions"
        );

        check(
            view.setView(0.0, 0.0, 100.0),
              "set valid view"
        );

        // -------------------------------------------------
        // Coordinate transformations
        // -------------------------------------------------

        near(
            view.worldToScreenX(0.0),
             400.0,
             1e-12,
             "center X"
        );

        near(
            view.worldToScreenY(0.0),
             300.0,
             1e-12,
             "center Y"
        );

        near(
            view.worldToScreenX(2.0),
             600.0,
             1e-12,
             "+X to right"
        );

        near(
            view.worldToScreenY(2.0),
             100.0,
             1e-12,
             "+Y upward"
        );

        near(
            view.screenToWorldX(250.0),
             -1.5,
             1e-12,
             "inverse X"
        );

        near(
            view.screenToWorldY(450.0),
             -1.5,
             1e-12,
             "inverse Y"
        );

        // -------------------------------------------------
        // Coordinate round trips
        // -------------------------------------------------

        check(
            view.setView(12.25, -7.5, 83.0),
              "offset view"
        );

        for (double coordinate : {
            -25.5, -3.25, 0.0, 1.5, 40.0
        }) {
            near(
                view.screenToWorldX(
                    view.worldToScreenX(coordinate)
                ),
                 coordinate,
                 1e-12,
                 "X coordinate round trip"
            );

            near(
                view.screenToWorldY(
                    view.worldToScreenY(coordinate)
                ),
                 coordinate,
                 1e-12,
                 "Y coordinate round trip"
            );
        }

        // -------------------------------------------------
        // Viewport resizing
        // -------------------------------------------------

        // Resizing moves the screen origin without
        // changing the world center.
        const double centerXBeforeResize =
        view.centerX();

        const double centerYBeforeResize =
        view.centerY();

        check(
            view.setDimensions(1200, 900),
              "resize dimensions"
        );

        near(
            view.screenToWorldX(600.0),
             centerXBeforeResize,
             1e-12,
             "resize preserves centered world X"
        );

        near(
            view.screenToWorldY(450.0),
             centerYBeforeResize,
             1e-12,
             "resize preserves centered world Y"
        );

        check(
            view.setDimensions(800, 600),
              "restore dimensions"
        );

        // -------------------------------------------------
        // Panning
        // -------------------------------------------------

        // Pan shifts the drawing in the direction
        // of the mouse drag.
        const double xBefore =
        view.worldToScreenX(2.0);

        const double yBefore =
        view.worldToScreenY(3.0);

        check(
            view.panPixels(75.0, -30.0),
              "valid pan"
        );

        near(
            view.worldToScreenX(2.0),
             xBefore + 75.0,
             1e-10,
             "drag right shifts plot right"
        );

        near(
            view.worldToScreenY(3.0),
             yBefore - 30.0,
             1e-10,
             "drag up shifts plot up"
        );

        // -------------------------------------------------
        // Cursor-anchored zoom
        // -------------------------------------------------

        for (int i = 0; i < 20; ++i) {
            const double anchorX =
            view.screenToWorldX(123.0);

            const double anchorY =
            view.screenToWorldY(456.0);

            check(
                view.zoomAt(
                    i % 2 == 0 ? 1.25 : 0.8,
                    123.0,
                    456.0
                ),
                "valid anchored zoom"
            );

            near(
                view.screenToWorldX(123.0),
                 anchorX,
                 1e-11,
                 "zoom preserves pointer world X"
            );

            near(
                view.screenToWorldY(456.0),
                 anchorY,
                 1e-11,
                 "zoom preserves pointer world Y"
            );
        }

        // -------------------------------------------------
        // Scale limits
        // -------------------------------------------------

        // Existing mouse-wheel zoom limits are preserved.
        check(
            view.zoomAt(1e100, 400.0, 300.0),
              "large zoom clamps"
        );

        near(
            view.pixelsPerUnit(),
             GraphViewport::kMaxWheelScale,
             0.0,
             "maximum wheel zoom"
        );

        check(
            view.zoomAt(1e-100, 400.0, 300.0),
              "small zoom clamps"
        );

        near(
            view.pixelsPerUnit(),
             GraphViewport::kMinWheelScale,
             0.0,
             "minimum wheel zoom"
        );

        // Programmatic setView retains the
        // baseline's broader valid range.
        check(
            view.setView(
                0.0,
                0.0,
                GraphViewport::kMinScale
            ),
            "minimum programmatic scale"
        );

        check(
            view.setView(
                0.0,
                0.0,
                GraphViewport::kMaxScale
            ),
            "maximum programmatic scale"
        );

        // -------------------------------------------------
        // Invalid input handling
        // -------------------------------------------------

        const double inf =
        std::numeric_limits<double>::infinity();

        const double nan =
        std::numeric_limits<double>::quiet_NaN();

        const GraphViewport saved = view;

        check(
            !view.setDimensions(0, 200),
              "reject zero width"
        );

        check(
            !view.setDimensions(-1, 200),
              "reject negative width"
        );

        check(
            !view.setDimensions(200, 0),
              "reject zero height"
        );

        check(
            !view.setView(nan, 0.0, 100.0),
              "reject NaN X center"
        );

        check(
            !view.setView(0.0, inf, 100.0),
              "reject infinite Y center"
        );

        check(
            !view.setView(0.0, 0.0, 0.0),
              "reject zero scale"
        );

        check(
            !view.setView(0.0, 0.0, -1.0),
              "reject negative scale"
        );

        check(
            !view.setView(0.0, 0.0, inf),
              "reject infinite scale"
        );

        check(
            !view.setView(
                0.0,
                0.0,
                GraphViewport::kMaxScale + 1
            ),
            "reject out-of-range programmatic scale"
        );

        check(
            !view.panPixels(nan, 10.0),
              "reject NaN pan"
        );

        check(
            !view.panPixels(10.0, inf),
              "reject infinite pan"
        );

        check(
            !view.zoomAt(0.0, 10.0, 20.0),
              "reject zero zoom"
        );

        check(
            !view.zoomAt(-1.0, 10.0, 20.0),
              "reject negative zoom"
        );

        check(
            !view.zoomAt(inf, 10.0, 20.0),
              "reject infinite zoom"
        );

        check(
            !view.zoomAt(2.0, nan, 20.0),
              "reject NaN pointer"
        );

        sameState(
            view,
            saved,
            "rejected operations preserve all state"
        );

        // -------------------------------------------------
        // Overflow handling
        // -------------------------------------------------

        // Overflow in navigation must leave the
        // viewport unchanged.
        check(
            view.setView(
                1.79e308,
                -1.79e308,
                100.0
            ),
            "very large center allowed"
        );

        const GraphViewport huge = view;

        check(
            !view.panPixels(-1e308, -1e308),
              "overflowing pan rejected"
        );

        sameState(
            view,
            huge,
            "overflowing pan preserves state"
        );

        // -------------------------------------------------
        // Results
        // -------------------------------------------------

        std::cout
        << "PASS: "
        << checks
        << " GraphViewport assertions\n";

        return 0;

    } catch (const std::exception& error) {
        std::cerr
        << "FAIL: "
        << error.what()
        << '\n';

        return 1;
    }
}
