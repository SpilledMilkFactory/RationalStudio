#include "GraphCanvas.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QImage>
#include <QTimer>
#include <QWheelEvent>

#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace rationalstudio;

namespace {
std::size_t assertions = 0;
void require(bool condition, const char* description) {
    if (!condition) throw std::runtime_error(description);
    ++assertions;
}
QImage capture(GraphCanvas& canvas) {
    QImage image(canvas.size(), QImage::Format_ARGB32_Premultiplied);
    require(!image.isNull(), "offscreen image allocated");
    image.fill(Qt::transparent);
    canvas.render(&image);
    require(image.pixelColor(image.width()/2, image.height()/2).alpha() == 255,
            "graph canvas produces opaque painted output");
    return image;
}
void advanceEvents(int milliseconds) {
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}
void wheel(GraphCanvas& canvas, int delta, QPointF at) {
    QWheelEvent event(at, at, QPoint(), QPoint(0, delta),
                      Qt::NoButton, Qt::NoModifier,
                      Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(&canvas, &event);
    require(event.isAccepted(), "synthetic wheel event accepted");
}
std::shared_ptr<const Expression> expression(const char* source) {
    auto parsed = std::make_shared<Expression>(source);
    require(parsed->valid(), "regression expression parsed");
    return parsed;
}
GraphCurve curve(std::shared_ptr<const Expression> f, QColor color,
                 bool enabled, const char* label) {
    return GraphCurve{color, std::move(f), enabled, QString::fromUtf8(label)};
}
}

int main(int argc, char** argv) {
    try {
        QApplication app(argc, argv);
        GraphCanvas canvas;
        canvas.resize(801, 601);
        canvas.show();
        app.processEvents();
        const auto sine = expression("sin(x)");
        const auto parabola = expression("x^2");
        canvas.setCurves({curve(sine, QColor("red"), true, "sine"),
                          curve(parabola, QColor("cyan"), true, "quadratic")});
        (void)capture(canvas);
        const std::size_t initial = canvas.geometryBuildCount();
        require(initial >= 2, "both mathematical curves built initially");
        const std::size_t evaluations = canvas.samplingDiagnostics().totalEvaluations;
        require(evaluations > 0, "initial rendering used mathematical sampler");

        // Exact same compiled expressions but different presentation order,
        // colors, labels and visibility must retain cached paths.
        canvas.setCurves({curve(parabola, QColor("green"), false, "hidden curve"),
                          curve(sine, QColor("yellow"), true, "renamed curve")});
        canvas.setGridVisible(false);
        canvas.setAxesVisible(false);
        (void)capture(canvas);
        require(canvas.geometryBuildCount() == initial,
                "presentation edits and ordering reuse existing geometry");
        require(canvas.samplingDiagnostics().totalEvaluations == evaluations,
                "presentation edits do not resample mathematical callbacks");

        // An invalid viewport request is an atomic no-op, including cache.
        const auto nan = std::numeric_limits<double>::quiet_NaN();
        const auto inf = std::numeric_limits<double>::infinity();
        canvas.setView(nan, 0, 58);
        canvas.setView(0, inf, 58);
        canvas.setView(0, 0, nan);
        (void)capture(canvas);
        require(canvas.centerX() == 0 && canvas.centerY() == 0 &&
                canvas.pixelsPerUnit() == 58,
                "invalid view retains original state");
        require(canvas.geometryBuildCount() == initial,
                "invalid view does not invalidate cached geometry");

        canvas.setCurves({curve(sine, QColor("red"), true, "sine"),
                          curve(parabola, QColor("cyan"), true, "quadratic")});
        (void)capture(canvas);
        require(canvas.geometryBuildCount() == initial,
                "reenabled curve uses its original cached path");

        // Programmatic viewport change causes one rebuild per visible curve.
        canvas.setView(0.5, -0.25, 75.0);
        (void)capture(canvas);
        require(canvas.geometryBuildCount() == initial + 2,
                "changed viewport regenerates both visible curve paths");
        const auto beforeNavigation = canvas.geometryBuildCount();
        const auto beforeEvaluation = canvas.samplingDiagnostics().totalEvaluations;
        const auto sched = canvas.schedulingDiagnostics();
        for (int i = 0; i < 12; ++i)
            wheel(canvas, 120, QPointF(540.25, 181.75));
        (void)capture(canvas);
        require(canvas.geometryBuildCount() == beforeNavigation,
                "navigation preview does not regenerate paths before timer");
        require(canvas.samplingDiagnostics().totalEvaluations == beforeEvaluation,
                "navigation preview does not invoke curve sampler");
        require(canvas.schedulingDiagnostics().coalescedNavigationEvents >=
                sched.coalescedNavigationEvents + 11,
                "rapid wheel events coalesce into a navigation burst");
        advanceEvents(55);
        (void)capture(canvas);
        require(canvas.geometryBuildCount() == beforeNavigation + 2,
                "latest zoom viewport sampled after navigation timer");

        // Rapid resizes cancel obsolete navigation work and recover a valid
        // image with newly sized viewport geometry.
        for (auto dims : {QSize(640, 480), QSize(975, 520), QSize(801, 601)}) {
            canvas.resize(dims);
            app.processEvents();
            const auto image = capture(canvas);
            require(image.size() == dims, "offscreen dimensions match widget");
        }

        // Small configured callback budgets still cap actual work. Deferred
        // curves are allowed to populate on later paints; counts are not FPS.
        require(!canvas.setSamplingLimits({3, 32}),
                "invalid per-curve budget rejected");
        require(canvas.setSamplingLimits({8, 8}),
                "tight per-curve and paint budgets accepted");
        (void)capture(canvas);
        require(canvas.samplingDiagnostics().lastPaintEvaluations <= 8,
                "single paint respects shared evaluation budget");
        for (int i = 0; i < 8; ++i) {
            app.processEvents();
            (void)capture(canvas);
            require(canvas.samplingDiagnostics().lastPaintEvaluations <= 8,
                    "deferred work never breaks shared callback ceiling");
        }
        require(canvas.setSamplingLimits({8192, 32768}),
                "default budgets restore successfully");
        (void)capture(canvas);
        require(canvas.samplingDiagnostics().lastPaintEvaluations <= 32768,
                "restored paint remains within configured budget");

        // Domain-restricted and pole-bearing expressions are renderable,
        // without relying on version-specific raster pixel comparisons.
        canvas.setCurves({curve(expression("sqrt(x)"), QColor("magenta"), true, "root"),
                          curve(expression("1/(x-0.133)"), QColor("green"), true, "pole")});
        (void)capture(canvas);
        require(canvas.geometryBuildCount() >= beforeNavigation + 2,
                "domain/discontinuity formulas accepted and rendered");
        canvas.setView(1.79e308, -1.79e308, 58);
        (void)capture(canvas);
        require(canvas.centerX() == 1.79e308,
                "extreme finite viewport center handled without crash");
        canvas.resetView();
        (void)capture(canvas);
        std::cout << "PASS: " << assertions << " GraphCanvasRegressionMatrix assertions\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return 1;
    }
}
