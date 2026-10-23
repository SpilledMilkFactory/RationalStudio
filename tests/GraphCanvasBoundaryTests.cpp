#include "GraphCanvas.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QImage>
#include <QResizeEvent>
#include <QWheelEvent>

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

using namespace rationalstudio;
namespace {
std::size_t assertions = 0;
void check(bool value, const char* description) {
    if (!value) throw std::runtime_error(description);
    ++assertions;
}
QImage capture(GraphCanvas& canvas) {
    QImage image(canvas.size(), QImage::Format_ARGB32_Premultiplied);
    check(!image.isNull(), "capture has positive dimensions");
    image.fill(Qt::transparent);
    canvas.render(&image);
    check(image.pixelColor(image.width() / 2, image.height() / 2).alpha() == 255,
          "canvas paints an opaque frame");
    return image;
}
}

int main(int argc, char** argv) {
    try {
        QApplication app(argc, argv);
        GraphCanvas canvas;
        canvas.resize(800, 600);
        canvas.show();
        app.processEvents();
        auto function = std::make_shared<Expression>("sin(x)");
        check(function->valid(), "test expression parses");
        canvas.setCurves({GraphCurve{QColor("red"), function, true, "sin(x)"}});
        (void)capture(canvas);
        const auto initial = canvas.geometryBuildCount();
        check(initial >= 1, "initial geometry was built");

        // Programmatic rejected view requests must be completely atomic.
        const double nan = std::numeric_limits<double>::quiet_NaN();
        const double inf = std::numeric_limits<double>::infinity();
        for (double invalid : {nan, inf, -inf}) {
            canvas.setView(invalid, 0, 58);
            canvas.setView(0, invalid, 58);
            canvas.setView(0, 0, invalid);
            check(canvas.centerX() == 0 && canvas.centerY() == 0 &&
                  canvas.pixelsPerUnit() == 58,
                  "invalid view left valid viewport unchanged");
        }
        (void)capture(canvas);
        check(canvas.geometryBuildCount() == initial,
              "rejected view changes did not invalidate paths");

        for (double scale : {GraphViewport::kMinScale,
                             GraphViewport::kMaxScale}) {
            canvas.setView(0, 0, scale);
            (void)capture(canvas);
            check(canvas.pixelsPerUnit() == scale,
                  "finite extreme zoom applied");
        }

        // The extreme allowed finite centers can overflow a screen-space
        // projection of world zero. Grid, axes and hover must not forward
        // such a coordinate to the raster painter or cast it to int.
        canvas.setView(1.79e308, -1.79e308, 58);
        (void)capture(canvas);
        check(canvas.centerX() == 1.79e308,
              "backwards-compatible extreme center retained");
        canvas.setView(0, 0, 58);
        (void)capture(canvas);

        // A resize invalidates stale paths and preserves the world view.
        const double cx = canvas.centerX();
        const double cy = canvas.centerY();
        const double scale = canvas.pixelsPerUnit();
        for (int i = 0; i < 16; ++i) {
            canvas.resize(350 + i * 17, 300 + i * 13);
            app.processEvents();
            (void)capture(canvas);
            check(canvas.centerX() == cx && canvas.centerY() == cy &&
                  canvas.pixelsPerUnit() == scale,
                  "resizing preserves world center and zoom");
        }

        // Resize during navigation must cancel an old pending timer;
        // subsequent rendering must use the new widget dimensions.
        const QPointF at(canvas.width() * 0.5, canvas.height() * 0.5);
        QWheelEvent wheel(at, at, QPoint(), QPoint(0, 120),
                          Qt::NoButton, Qt::NoModifier,
                          Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(&canvas, &wheel);
        check(canvas.schedulingDiagnostics().navigationEvents > 0,
              "navigation request registered");
        canvas.resize(930, 510);
        app.processEvents();
        (void)capture(canvas);
        const auto buildCount = canvas.geometryBuildCount();
        app.processEvents();
        (void)capture(canvas);
        check(canvas.geometryBuildCount() == buildCount,
              "obsolete navigation timeout did not rebuild after resize");

        // A synthetic invalid transient resize must not mutate the last
        // valid viewport, and a valid resize can subsequently recover.
        QResizeEvent zero(QSize(0, 0), canvas.size());
        QCoreApplication::sendEvent(&canvas, &zero);
        (void)capture(canvas); // widget has not actually changed size
        canvas.resize(931, 511);
        app.processEvents();
        (void)capture(canvas);
        check(canvas.centerX() == cx && canvas.centerY() == cy,
              "viewport recovers following zero-sized notification");

        std::cout << "PASS: " << assertions
                  << " GraphCanvasBoundary assertions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
