#include "GraphViewport.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using rationalstudio::GraphViewport;

namespace {
std::size_t assertions = 0;

void require(bool condition, const std::string& description) {
    if (!condition) throw std::runtime_error(description);
    ++assertions;
}

void approximately(double actual, double expected, double absoluteTolerance,
                   const std::string& description) {
    require(std::isfinite(actual) && std::isfinite(expected) &&
            std::abs(actual - expected) <= absoluteTolerance, description);
}

void sameState(const GraphViewport& actual, const GraphViewport& expected,
               const std::string& description) {
    require(actual.width() == expected.width() &&
            actual.height() == expected.height() &&
            actual.centerX() == expected.centerX() &&
            actual.centerY() == expected.centerY() &&
            actual.pixelsPerUnit() == expected.pixelsPerUnit(), description);
}

GraphViewport makeView(double cx = 0.0, double cy = 0.0,
                       double scale = 100.0) {
    GraphViewport view;
    require(view.setDimensions(801, 601), "dimensions configured");
    require(view.setView(cx, cy, scale), "view configured");
    return view;
}

void coordinateRoundTrips() {
    // Odd dimensions exercise half-pixel centers (not just integer centers).
    for (double scale : {0.1, 0.5, 58.0, 100.0, 5000.0, 20000.0}) {
        for (double cx : {-1e8, -1.25, 0.0, 2.75, 1e8}) {
            const auto view = makeView(cx, -cx + 3.5, scale);
            for (double sx : {0.0, 0.5, 125.25, 400.5, 679.75, 801.0}) {
                const double world = view.screenToWorldX(sx);
                approximately(view.worldToScreenX(world), sx,
                              std::max(1e-7, std::abs(cx) * scale * 4e-16),
                              "X roundtrip under varied view states");
            }
            for (double sy : {0.0, 0.5, 123.25, 300.5, 480.75, 601.0}) {
                const double world = view.screenToWorldY(sy);
                approximately(view.worldToScreenY(world), sy,
                              std::max(1e-7, std::abs(cx) * scale * 4e-16),
                              "Y roundtrip under varied view states");
            }
        }
    }
}

void anchoredZoomStability() {
    auto view = makeView(1234.25, -567.75, 125.0);
    const double sx = 647.25;
    const double sy = 127.75;
    const double anchorX = view.screenToWorldX(sx);
    const double anchorY = view.screenToWorldY(sy);

    for (int i = 0; i < 500; ++i) {
        require(view.zoomAt(1.12, sx, sy), "zoom in succeeds");
        require(view.zoomAt(1.0 / 1.12, sx, sy), "zoom out succeeds");
    }

    approximately(view.pixelsPerUnit(), 125.0, 1e-9,
                  "paired zoom returns to initial scale");
    approximately(view.screenToWorldX(sx), anchorX, 1e-9,
                  "zoom cycling preserves X anchor");
    approximately(view.screenToWorldY(sy), anchorY, 1e-9,
                  "zoom cycling preserves Y anchor");

    // At a clamped zoom limit the state, including center, must not drift.
    require(view.setView(5.25, -6.5, GraphViewport::kMaxWheelScale),
            "set upper wheel scale");
    const GraphViewport upper = view;
    for (int i = 0; i < 500; ++i)
        require(view.zoomAt(1.12, sx, sy), "upper clamp operation succeeds");
    sameState(view, upper, "repeated upper-bound zoom preserves state exactly");

    require(view.setView(5.25, -6.5, GraphViewport::kMinWheelScale),
            "set lower wheel scale");
    const GraphViewport lower = view;
    for (int i = 0; i < 500; ++i)
        require(view.zoomAt(1.0 / 1.12, sx, sy), "lower clamp operation succeeds");
    sameState(view, lower, "repeated lower-bound zoom preserves state exactly");
}

void restoredScaleNavigation() {
    auto view = makeView(3.0, 4.0, GraphViewport::kMaxScale);
    const GraphViewport high = view;
    require(view.zoomAt(1.1, 600.0, 100.0), "zoom-in above wheel max is valid no-op");
    sameState(view, high, "zoom-in does not accidentally shrink large restored view");

    require(view.zoomAt(0.9, 600.0, 100.0), "zoom-out above wheel max succeeds");
    approximately(view.pixelsPerUnit(), 18000.0, 1e-9,
                  "zoom-out changes restored scale monotonically");

    require(view.setView(3.0, 4.0, GraphViewport::kMinScale),
            "minimum programmatic scale");
    const GraphViewport low = view;
    require(view.zoomAt(0.9, 600.0, 100.0), "zoom-out below wheel minimum no-op");
    sameState(view, low, "zoom-out does not accidentally enlarge tiny restored view");

    require(view.zoomAt(1.1, 600.0, 100.0), "zoom-in below wheel min succeeds");
    approximately(view.pixelsPerUnit(), 0.11, 1e-12,
                  "zoom-in progresses smoothly toward wheel range");

    require(view.setView(3.0, 4.0, GraphViewport::kMaxScale),
            "reset high programmatic scale");
    const GraphViewport untouched = view;
    require(view.zoomAt(1.0, 600.0, 100.0), "unit zoom factor succeeds");
    sameState(view, untouched, "unit zoom does not clamp or translate");
}

void panningAndRollback() {
    auto view = makeView(0.0, 0.0, 100.0);
    const double originalX = view.worldToScreenX(2.0);
    const double originalY = view.worldToScreenY(3.0);
    require(view.panPixels(75.5, -30.25), "pan succeeds");
    approximately(view.worldToScreenX(2.0), originalX + 75.5, 1e-11,
                  "screen X follows mouse drag");
    approximately(view.worldToScreenY(3.0), originalY - 30.25, 1e-11,
                  "screen Y follows mouse drag");

    const auto beforeZero = view;
    require(view.panPixels(0.0, 0.0), "zero pan succeeds");
    sameState(view, beforeZero, "zero pan is exact no-op");

    const auto saved = view;
    const double inf = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    require(!view.panPixels(nan, 10.0), "nan pan rejected");
    require(!view.panPixels(10.0, inf), "infinite pan rejected");
    require(!view.zoomAt(0.0, 300.0, 400.0), "zero zoom rejected");
    require(!view.zoomAt(-1.0, 300.0, 400.0), "negative zoom rejected");
    require(!view.zoomAt(inf, 300.0, 400.0), "infinite zoom rejected");
    require(!view.zoomAt(1.12, nan, 400.0), "nan cursor rejected");
    require(!view.zoomAt(1.12, 300.0, inf), "infinite cursor rejected");
    require(!view.setDimensions(0, 600), "zero width rejected");
    require(!view.setDimensions(800, -1), "negative height rejected");
    sameState(view, saved, "invalid operations preserve all viewport state");

    require(view.setView(1.79e308, -1.79e308, 100.0),
            "extreme centers remain allowed for backwards compatibility");
    const auto enormous = view;
    require(!view.panPixels(-1e308, -1e308),
            "overflowing pan rejected atomically");
    sameState(view, enormous, "overflowing pan cannot partially update state");

    require(view.setView(1e16, -1e16, 1000.0), "large center configured");
    const auto huge = view;
    require(!view.panPixels(1.0, 0.0),
            "unrepresentable one-pixel pan rejected");
    sameState(view, huge, "unrepresentable pan preserves state");
    require(!view.zoomAt(1.25, 700.0, 100.0),
            "unrepresentable off-center zoom translation rejected");
    sameState(view, huge, "failed anchored zoom is atomic");
    require(view.zoomAt(1.25, 400.5, 300.5),
            "center-anchored zoom remains possible at large coordinates");
    approximately(view.pixelsPerUnit(), 1250.0, 1e-12,
                  "center-anchored zoom changes scale");
}

void repeatedPanStability() {
    auto view = makeView(1e9 + 0.25, -(1e9 + 0.75), 1000.0);
    const auto original = view;
    for (int i = 0; i < 500; ++i) {
        require(view.panPixels(13.0, -7.0), "small drag succeeds");
        require(view.panPixels(-13.0, 7.0), "inverse drag succeeds");
    }
    approximately(view.centerX(), original.centerX(), 1e-5,
                  "repeated reverse pan maintains X center");
    approximately(view.centerY(), original.centerY(), 1e-5,
                  "repeated reverse pan maintains Y center");
}
} // namespace

int main() {
    try {
        coordinateRoundTrips();
        anchoredZoomStability();
        restoredScaleNavigation();
        panningAndRollback();
        repeatedPanStability();
        std::cout << "PASS: " << assertions
                  << " GraphViewportPrecision assertions\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
