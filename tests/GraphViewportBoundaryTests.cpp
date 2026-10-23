#include "GraphViewport.hpp"
#include "GraphGridLayout.hpp"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using rationalstudio::GraphViewport;
using rationalstudio::graphGridAxis;

namespace {
std::size_t assertions = 0;
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
    ++assertions;
}
void same(const GraphViewport& a, const GraphViewport& b) {
    check(a.width() == b.width() && a.height() == b.height() &&
          a.centerX() == b.centerX() && a.centerY() == b.centerY() &&
          a.pixelsPerUnit() == b.pixelsPerUnit(),
          "rejected viewport operation must preserve state atomically");
}
}

int main() {
    try {
        GraphViewport view;
        check(view.setDimensions(800, 600), "initial dimensions");
        check(view.setView(0.0, 0.0, 58.0), "initial view");
        const GraphViewport baseline = view;
        const double nan = std::numeric_limits<double>::quiet_NaN();
        const double inf = std::numeric_limits<double>::infinity();

        for (int bad : {0, -1, -100}) {
            check(!view.setDimensions(bad, 600), "nonpositive width rejected");
            same(view, baseline);
            check(!view.setDimensions(800, bad), "nonpositive height rejected");
            same(view, baseline);
        }
        for (double bad : {nan, inf, -inf}) {
            check(!view.setView(bad, 0.0, 58.0), "invalid centerX rejected");
            same(view, baseline);
            check(!view.setView(0.0, bad, 58.0), "invalid centerY rejected");
            same(view, baseline);
            check(!view.setView(0.0, 0.0, bad), "nonfinite scale rejected");
            same(view, baseline);
            check(!view.panPixels(bad, 1.0), "invalid pan rejected");
            same(view, baseline);
            check(!view.zoomAt(1.1, bad, 100.0), "invalid cursor rejected");
            same(view, baseline);
        }
        check(!view.setView(0, 0, 0.0), "zero scale rejected");
        same(view, baseline);
        check(!view.setView(0, 0, -1.0), "negative scale rejected");
        same(view, baseline);
        check(!view.zoomAt(0, 500, 300), "zero zoom rejected");
        same(view, baseline);
        check(!view.zoomAt(inf, 500, 300), "infinite zoom rejected");
        same(view, baseline);

        // Extreme finite centers must remain accepted for compatibility,
        // while grid labels intentionally disappear when precision is lost.
        check(view.setView(1.79e308, -1.79e308, 58.0),
              "extreme finite centers remain accepted");
        check(!graphGridAxis(view.centerX(), view.width(),
                             view.pixelsPerUnit()).has_value(),
              "huge center suppresses unreliable x grid");
        check(!graphGridAxis(view.centerY(), view.height(),
                             view.pixelsPerUnit()).has_value(),
              "huge center suppresses unreliable y grid");
        check(view.setView(0.0, 0.0, 58.0), "reset extreme center");

        // Test grid phase and spacing without ever converting huge screen
        // projections to integer pixel positions.
        for (double scale : {GraphViewport::kMinScale, 0.5, 1.0,
                             58.0, 1000.0, GraphViewport::kMaxScale}) {
            for (int extent : {1, 2, 350, 800, 4096}) {
                for (double center : {-1234.75, -13.125, 0.0, 2.3, 999.625}) {
                    const auto axis = graphGridAxis(center, extent, scale);
                    check(axis.has_value(), "moderate center/scale grid is valid");
                    check(std::isfinite(axis->firstPixel) &&
                          axis->firstPixel >= 0.0 &&
                          axis->firstPixel <= axis->spacingPixels,
                          "first grid line in initial grid interval");
                    check(axis->spacingPixels >= 4.0 &&
                          std::isfinite(axis->spacingPixels),
                          "pixel step is positive and representable");
                    check(std::isfinite(axis->firstWorld) &&
                          std::isfinite(axis->spacingWorld),
                          "world positions are finite");
                    for (int i = 0; i < 8; ++i) {
                        const double screen = std::fma(double(i),
                            axis->spacingPixels, axis->firstPixel);
                        const double world = std::fma(double(i),
                            axis->spacingWorld, axis->firstWorld);
                        const double actual = std::fma(world - center,
                            scale, extent * 0.5);
                        check(std::isfinite(screen) && std::isfinite(actual) &&
                              std::abs(screen - actual) < 0.25,
                              "grid lattice matches viewport projection");
                    }
                }
            }
        }
        for (double bad : {nan, inf, -inf}) {
            check(!graphGridAxis(bad, 800, 58.0), "invalid grid center rejected");
            check(!graphGridAxis(0.0, 800, bad), "invalid grid scale rejected");
        }
        check(!graphGridAxis(0, 0, 58), "zero-sized grid rejected");
        check(!graphGridAxis(0, -1, 58), "negative-sized grid rejected");
        check(!graphGridAxis(0, 800, 0), "zero scale grid rejected");
        check(!graphGridAxis(0, 800, -10), "negative scale grid rejected");

        // Width/height changes do not change world center or scale.
        for (int extent : {1, 2, 350, 600, 8192, 1000000}) {
            check(view.setDimensions(extent, extent), "valid resize accepted");
            check(view.centerX() == 0.0 && view.centerY() == 0.0 &&
                  view.pixelsPerUnit() == 58.0, "resize preserves world view");
            check(graphGridAxis(view.centerX(), view.width(),
                                view.pixelsPerUnit()).has_value(),
                  "grid remains valid after resize");
        }
        std::cout << "PASS: " << assertions
                  << " GraphViewportBoundary assertions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
