#include "BenchmarkMetrics.hpp"
#include "GraphCanvas.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QImage>
#include <QMouseEvent>
#include <QWheelEvent>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace rationalstudio;
using namespace rationalstudio::benchmark;
namespace {
constexpr int kWidth = 960;
constexpr int kHeight = 600;
constexpr auto kSettlementTimeout = std::chrono::seconds(10);

std::vector<std::shared_ptr<const Expression>> expressions() {
    std::vector<std::shared_ptr<const Expression>> result;
    for (const char* source : {"sin(x)", "x^2/12-2", "cos(3*x)",
                               "tanh(1000*(x-0.137))", "1/(x-0.133)",
                               "floor(x)", "atan(120*x)", "sqrt(x)"}) {
        auto expression = std::make_shared<Expression>(source);
        if (!expression->valid()) throw std::runtime_error(std::string("invalid expression: ") + source);
        result.push_back(std::move(expression));
    }
    return result;
}
std::vector<GraphCurve> curvesFor(
    const std::vector<std::shared_ptr<const Expression>>& parsed, std::size_t count) {
    std::vector<GraphCurve> output;
    const std::array<QColor, 8> colors = {
        QColor("#f46d6d"), QColor("#6dd3e8"), QColor("#fde68a"),
        QColor("#87f5a0"), QColor("#c6a7ff"), QColor("#ffac60"),
        QColor("#d8ecfc"), QColor("#e786bf")};
    for (std::size_t i = 0; i < count; ++i) {
        output.push_back({colors[i], parsed[i], true,
                          QString("benchmark curve %1").arg(static_cast<int>(i))});
    }
    return output;
}
void render(GraphCanvas& canvas, QImage& framebuffer) {
    canvas.render(&framebuffer); // QWidget::render includes paintEvent and rasterization
}
void sendWheel(GraphCanvas& canvas, QPointF local, int delta) {
    QWheelEvent e(local, canvas.mapToGlobal(local.toPoint()), QPoint(), QPoint(0, delta),
                  Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(&canvas, &e);
    if (!e.isAccepted()) throw std::runtime_error("wheel event rejected");
}
void sendMouse(GraphCanvas& canvas, QEvent::Type type, QPointF local,
               Qt::MouseButton button, Qt::MouseButtons buttons) {
    const QPointF global = canvas.mapToGlobal(local.toPoint());
    // Seven-argument overload: non-deprecated in Qt6; supported by Qt5.
    QMouseEvent e(type, local, local, global, button, buttons, Qt::NoModifier);
    QCoreApplication::sendEvent(&canvas, &e);
}
void measureNavigation(QApplication& app, GraphCanvas& canvas,
                       QImage& image, std::size_t curveCount,
                       const Options& options, std::vector<Measurement>& rows,
                       int iteration, bool record, bool pan) {
    // Ensure fully settled, ready paths before measuring interaction.
    canvas.resetView();
    render(canvas, image);
    const auto startBuilds = canvas.geometryBuildCount();
    const auto startEvals = canvas.samplingDiagnostics().totalEvaluations;
    const auto startSchedule = canvas.schedulingDiagnostics();
    const auto buildsBeforeEvents = canvas.geometryBuildCount();
    const auto evaluationsBeforeEvents = canvas.samplingDiagnostics().totalEvaluations;
    const auto previewBeforeEvents = canvas.schedulingDiagnostics().previewPaints;
    const auto inputStart = Clock::now();
    double firstPreviewMs = 0;
    double totalDispatchMs = 0;
    double totalPreviewPaintMs = 0;
    if (pan) {
        sendMouse(canvas, QEvent::MouseButtonPress, QPointF(400, 280),
                  Qt::LeftButton, Qt::LeftButton);
    }
    for (int event = 0; event < options.navigationEvents; ++event) {
        const auto before = Clock::now();
        if (pan) {
            sendMouse(canvas, QEvent::MouseMove,
                      QPointF(400 + (event + 1) * 2, 280 + (event + 1)),
                      Qt::NoButton, Qt::LeftButton);
        } else {
            sendWheel(canvas, QPointF(480, 270), 120);
        }
        const auto dispatched = Clock::now();
        totalDispatchMs += milliseconds(before, dispatched);
        render(canvas, image);
        const auto rendered = Clock::now();
        totalPreviewPaintMs += milliseconds(dispatched, rendered);
        if (event == 0) firstPreviewMs = milliseconds(inputStart, rendered);
    }
    if (canvas.geometryBuildCount() != buildsBeforeEvents ||
        canvas.samplingDiagnostics().totalEvaluations != evaluationsBeforeEvents ||
        canvas.schedulingDiagnostics().previewPaints <
            previewBeforeEvents + static_cast<std::size_t>(options.navigationEvents))
        throw std::runtime_error("navigation preview unexpectedly resampled or was not painted");
    if (pan) {
        sendMouse(canvas, QEvent::MouseButtonRelease,
                  QPointF(400 + options.navigationEvents * 2,
                          280 + options.navigationEvents),
                  Qt::LeftButton, Qt::NoButton);
    }
    const auto lastEvent = Clock::now();
    const auto deadline = lastEvent + kSettlementTimeout;
    while (Clock::now() < deadline) {
        app.processEvents(QEventLoop::AllEvents, 20);
        render(canvas, image);
        const auto scheduling = canvas.schedulingDiagnostics();
        if (scheduling.completedNavigationRefreshes >
                startSchedule.completedNavigationRefreshes &&
            canvas.geometryBuildCount() >= startBuilds + curveCount)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (Clock::now() >= deadline)
        throw std::runtime_error("navigation did not settle with fully rebuilt geometry");
    const auto final = Clock::now();
    const auto evals = canvas.samplingDiagnostics().totalEvaluations - startEvals;
    const auto builds = canvas.geometryBuildCount() - startBuilds;
    const std::string scenario = pan ? "mouse_drag_burst" : "wheel_zoom_burst";
    if (record) {
        rows.push_back({"input_to_first_preview", scenario, curveCount,
                        static_cast<std::size_t>(iteration), firstPreviewMs, 0, 0, 0, false});
        rows.push_back({"navigation_event_dispatch_total", scenario, curveCount,
                        static_cast<std::size_t>(iteration), totalDispatchMs, 0, 0, 0, false});
        rows.push_back({"preview_render_total", scenario, curveCount,
                        static_cast<std::size_t>(iteration), totalPreviewPaintMs, 0, 0, 0, false});
        rows.push_back({"last_event_to_geometry_settled", scenario, curveCount,
                        static_cast<std::size_t>(iteration), milliseconds(lastEvent, final),
                        evals, builds, 0, false});
    }
}
void measurePaints(QApplication& app, GraphCanvas& canvas, QImage& image,
                   std::size_t curveCount, const Options& options,
                   std::vector<Measurement>& rows) {
    for (int iteration = -options.warmups; iteration < options.repetitions; ++iteration) {
        canvas.setView(0.01 * (iteration + 100), 0.0, 58.0);
        const auto beforeBuilds = canvas.geometryBuildCount();
        const auto beforeEvals = canvas.samplingDiagnostics().totalEvaluations;
        const auto begin = Clock::now();
        render(canvas, image);
        const auto end = Clock::now();
        const auto built = canvas.geometryBuildCount() - beforeBuilds;
        const auto sampled = canvas.samplingDiagnostics().totalEvaluations - beforeEvals;
        if (iteration >= 0) rows.push_back({
            "cold_invalidated_render", "960x600", curveCount,
            static_cast<std::size_t>(iteration), milliseconds(begin, end),
            sampled, built, 0, false});
        if (built == 0 || sampled == 0)
            throw std::runtime_error("cold scene produced no geometry or sampling work");
        const auto readyBuilds = canvas.geometryBuildCount();
        const auto readyEvals = canvas.samplingDiagnostics().totalEvaluations;
        const auto warmBegin = Clock::now();
        render(canvas, image);
        const auto warmEnd = Clock::now();
        if (canvas.geometryBuildCount() != readyBuilds ||
            canvas.samplingDiagnostics().totalEvaluations != readyEvals)
            throw std::runtime_error("warm repaint unexpectedly resampled curves");
        if (iteration >= 0) rows.push_back({
            "warm_cached_render", "960x600", curveCount,
            static_cast<std::size_t>(iteration), milliseconds(warmBegin, warmEnd),
            0, 0, 0, false});
        app.processEvents();
    }
}
}
int main(int argc, char** argv) {
    try {
        // QApplication may consume toolkit args; remove benchmark CLI flags first.
        auto opts = parseOptions(argc, argv);
        int qtArgc = 1;
        QApplication app(qtArgc, argv);
        const auto parsed = expressions();
        std::vector<Measurement> rows;
        for (std::size_t count : {std::size_t{1}, std::size_t{4}, std::size_t{8}}) {
            GraphCanvas canvas;
            canvas.resize(kWidth, kHeight);
            canvas.show();
            app.processEvents();
            canvas.setCurves(curvesFor(parsed, count));
            QImage framebuffer(canvas.size(), QImage::Format_ARGB32_Premultiplied);
            if (framebuffer.isNull()) throw std::runtime_error("could not allocate framebuffer");
            framebuffer.fill(Qt::black);
            render(canvas, framebuffer);
            measurePaints(app, canvas, framebuffer, count, opts, rows);
            for (int iter = -opts.warmups; iter < opts.repetitions; ++iter) {
                measureNavigation(app, canvas, framebuffer, count,
                                  opts, rows, iter, iter >= 0, false);
                measureNavigation(app, canvas, framebuffer, count,
                                  opts, rows, iter, iter >= 0, true);
            }
        }
        writeMeasurements(opts.output, "graph_canvas", rows,
            std::string("Qt ") + qVersion() +
            "; offscreen QWidget::render to preallocated 960x600 image; "
            "cold includes sampling+QPainter rasterization; warm isolates cached "
            "raster work; preview is event-to-offscreen-render, not screen presentation; "
            "last-event-to-settled includes scheduler timer, event loop, sampling "
            "and paint. Synthetic events; not physical-input-to-display latency. "
            "No hard FPS or frame-time claim. Navigation burst size set on command line.");
        std::cout << "Graph canvas benchmark complete: " << rows.size()
                  << " samples; CSV/JSON in " << opts.output << '\n';
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Graph canvas benchmark: " << ex.what() << '\n';
        return 1;
    }
}
