#include "GraphCanvas.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QImage>
#include <QMouseEvent>
#include <QTimer>
#include <QWheelEvent>

#include <iostream>
#include <memory>
#include <stdexcept>

using namespace rationalstudio;

namespace {
int assertions = 0;
void require(bool condition, const char* what) {
    if (!condition) throw std::runtime_error(what);
    ++assertions;
}
void paint(GraphCanvas& canvas) {
    QImage image(canvas.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    canvas.render(&image);
    require(!image.isNull(), "valid offscreen image");
}
bool hasRedCurvePixel(const QImage& image) {
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.red() > 170 && color.red() > color.green() * 2 &&
                color.red() > color.blue() * 2) return true;
        }
    }
    return false;
}
void advanceEvents(int milliseconds) {
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}
void wheel(GraphCanvas& widget, int angle, const QPointF& at) {
    QWheelEvent event(at, at, QPoint(), QPoint(0, angle),
                      Qt::NoButton, Qt::NoModifier,
                      Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(&widget, &event);
    require(event.isAccepted(), "synthetic wheel input was consumed");
}
GraphCurve curve(const char* formula, const QColor& color) {
    auto expression = std::make_shared<Expression>(formula);
    require(expression->valid(), "test expression parses");
    return GraphCurve{color, expression, true, QString::fromUtf8(formula)};
}
}

int main(int argc, char** argv) {
    try {
        QApplication application(argc, argv);
        GraphCanvas canvas;
        canvas.resize(800, 600);
        canvas.show();
        application.processEvents();
        canvas.setCurves({curve("sin(x)", QColor("red")),
                          curve("x^2", QColor("blue"))});
        paint(canvas);
        const auto initial = canvas.geometryBuildCount();
        require(initial >= 2, "initial paths constructed");
        const auto initialEval = canvas.samplingDiagnostics().totalEvaluations;
        const auto before = canvas.schedulingDiagnostics();

        // Simulate many wheel events without pumping the Qt event loop. No
        // intermediate viewport may trigger mathematical sampling.
        for (int i = 0; i < 30; ++i) wheel(canvas, 120, QPointF(560, 245));
        require(canvas.schedulingDiagnostics().navigationEvents ==
                before.navigationEvents + 30, "30 viewport changes recorded");
        require(canvas.schedulingDiagnostics().coalescedNavigationEvents >=
                before.coalescedNavigationEvents + 29,
                "navigation requests are coalesced");
        paint(canvas);
        require(canvas.geometryBuildCount() == initial,
                "preview paint did not rebuild paths");
        require(canvas.samplingDiagnostics().totalEvaluations == initialEval,
                "preview did not invoke the mathematical sampler");
        require(canvas.schedulingDiagnostics().previewPaints > before.previewPaints,
                "preview path rendered while navigation outstanding");

        advanceEvents(45);
        paint(canvas);
        require(canvas.geometryBuildCount() == initial + 2,
                "one complete regeneration for the latest viewport");
        require(canvas.schedulingDiagnostics().completedNavigationRefreshes ==
                before.completedNavigationRefreshes + 1,
                "one navigation refresh for thirty inputs");
        const auto cached = canvas.geometryBuildCount();
        paint(canvas);
        require(canvas.geometryBuildCount() == cached,
                "ordinary repaint reuses cached geometry");

        // Programmatic view replacement remains immediate and cancels any
        // obsolete navigation timeout without duplicating work later.
        wheel(canvas, -120, QPointF(300, 220));
        canvas.setView(0, 0, 58);
        paint(canvas);
        const auto afterSetView = canvas.geometryBuildCount();
        require(afterSetView == cached + 2,
                "programmatic viewport changes remain synchronous");
        advanceEvents(45);
        paint(canvas);
        require(canvas.geometryBuildCount() == afterSetView,
                "superseded navigation callback cannot rebuild again");

        // Appearance-only paint must not recreate the geometry.
        canvas.setGridVisible(false);
        canvas.setAxesVisible(false);
        paint(canvas);
        require(canvas.geometryBuildCount() == afterSetView,
                "appearance-only updates preserve cache");

        // Dragging coalesces viewport changes as well; the cheap preview
        // remains available even while hundreds of pointer events arrive.
        const auto dragBefore = canvas.geometryBuildCount();
        const double oldX = canvas.centerX();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(400, 300),
                          QPointF(400, 300),
                          QPointF(canvas.mapToGlobal(QPoint(400, 300))),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
#else
        QMouseEvent press(QEvent::MouseButtonPress, QPointF(400, 300),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
#endif
        QCoreApplication::sendEvent(&canvas, &press);
        for (int i = 1; i <= 12; ++i) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QMouseEvent move(QEvent::MouseMove, QPointF(400 + i * 3, 300),
                             QPointF(400 + i * 3, 300),
                             QPointF(canvas.mapToGlobal(QPoint(400 + i * 3, 300))),
                             Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
#else
            QMouseEvent move(QEvent::MouseMove, QPointF(400 + i * 3, 300),
                             Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
#endif
            QCoreApplication::sendEvent(&canvas, &move);
        }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QMouseEvent release(QEvent::MouseButtonRelease, QPointF(436, 300),
                            QPointF(436, 300),
                            QPointF(canvas.mapToGlobal(QPoint(436, 300))),
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
#else
        QMouseEvent release(QEvent::MouseButtonRelease, QPointF(436, 300),
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
#endif
        QCoreApplication::sendEvent(&canvas, &release);
        require(canvas.centerX() != oldX, "drag changed viewport center");
        paint(canvas);
        require(canvas.geometryBuildCount() == dragBefore,
                "drag preview does not resample");
        advanceEvents(45);
        paint(canvas);
        require(canvas.geometryBuildCount() == dragBefore + 2,
                "drag refresh resampled both curves after coalescing");

        // Regression: expiry may occur while QWidget updates are disabled.
        // If another input arrives before the queued refresh paint, ready
        // preview geometry MUST remain available (not flash to blank).
        canvas.setView(0, 0, 58);
        paint(canvas);
        const auto beforeInterleavedInput = canvas.geometryBuildCount();
        canvas.setUpdatesEnabled(false);
        wheel(canvas, 120, QPointF(560, 245));
        advanceEvents(45); // scheduler timer expires; Qt cannot paint here
        wheel(canvas, 120, QPointF(560, 245));
        canvas.setUpdatesEnabled(true);
        QImage preview(canvas.size(), QImage::Format_ARGB32_Premultiplied);
        preview.fill(Qt::transparent);
        canvas.render(&preview);
        require(canvas.geometryBuildCount() == beforeInterleavedInput,
                "new input before queued paint preserves old geometry for preview");
        require(hasRedCurvePixel(preview),
                "coalesced preview still paints the red curve after timer/input race");
        advanceEvents(45);
        paint(canvas);
        require(canvas.geometryBuildCount() == beforeInterleavedInput + 2,
                "eventually rebuild paths for final viewport after timer/input race");

        std::cout << "PASS: " << assertions << " GraphCanvasScheduling assertions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
