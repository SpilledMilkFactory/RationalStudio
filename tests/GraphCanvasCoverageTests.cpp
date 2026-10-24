#include "GraphCanvas.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QImage>
#include <QTimer>
#include <QWheelEvent>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <vector>

using namespace rationalstudio;
namespace {
std::size_t assertions = 0;
void require(bool condition, const char* message) {
    ++assertions;
    if (!condition) throw std::runtime_error(message);
}
QImage paint(GraphCanvas& canvas) {
    QImage image(canvas.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    canvas.render(&image);
    require(canvas.samplingDiagnostics().lastPaintEvaluations <=
            canvas.samplingLimits().maxEvaluationsPerPaint, "paint callback limit");
    return image;
}
QImage settle(GraphCanvas& canvas) {
    for (int frame = 0; frame < 2000; ++frame) {
        auto image = paint(canvas);
        if (canvas.samplingDiagnostics().lastPaintDeferredCurves == 0) return image;
    }
    throw std::runtime_error("canvas sampling did not finish");
}
bool red(const QImage& image, int x, int y) {
    const auto pixel = image.pixelColor(x, y);
    return pixel.red() > 160 && pixel.red() > pixel.green() * 2;
}
void branches(const QImage& image, const GraphCanvas& canvas) {
    for (int y : {3, image.height()-35}) {
        const double worldY = canvas.centerY() + (image.height()*0.5-y)/canvas.pixelsPerUnit();
        const double phase = std::atan(worldY);
        const double left = canvas.centerX() - image.width()*0.5/canvas.pixelsPerUnit();
        const double right = canvas.centerX() + image.width()*0.5/canvas.pixelsPerUnit();
        const int first = static_cast<int>(std::floor((left-phase)/std::numbers::pi));
        const int last = static_cast<int>(std::ceil((right-phase)/std::numbers::pi));
        for (int k = first; k <= last; ++k) {
            const double x = image.width()*0.5 +
                (phase+k*std::numbers::pi-canvas.centerX())*canvas.pixelsPerUnit();
            if (x < 4 || x > image.width()-4) continue;
            bool found = false;
            for (int dx = -3; dx <= 3; ++dx)
                for (int dy = -1; dy <= 1; ++dy)
                    found = found || red(image, static_cast<int>(std::round(x))+dx, y+dy);
            require(found, "every tangent tail is rasterized at top and bottom");
        }
    }
}
void waitForRefresh() {
    QEventLoop loop;
    QTimer::singleShot(40, &loop, &QEventLoop::quit);
    loop.exec();
}
}
int main(int argc, char** argv) {
    try {
        QApplication app(argc, argv);
        GraphCanvas canvas;
        canvas.resize(900, 617);
        canvas.setGridVisible(false);
        canvas.setAxesVisible(false);
        canvas.show();
        app.processEvents();
        const auto tangent = std::make_shared<Expression>("tan(x)");
        const auto reciprocal = std::make_shared<Expression>("1/(x-0.133)");
        const auto floor = std::make_shared<Expression>("floor(x)");
        canvas.setCurves({{QColor("red"), tangent, true, "tan(x)"},
                          {QColor("cyan"), reciprocal, true, "1/(x-0.133)"},
                          {QColor("green"), floor, true, "floor(x)"}});
        const bool dpiSmoke = argc > 1;
        const std::vector<int> widths = dpiSmoke ? std::vector<int>{900} : std::vector<int>{350, 900, 1600};
        const std::vector<double> scales = dpiSmoke ? std::vector<double>{9., 1.38} :
                                                    std::vector<double>{58., 9., 1.38, .5, .1};
        for (int width : widths) {
            canvas.resize(width, 617);
            app.processEvents();
            for (double scale : scales) {
                canvas.setView(13.7, 0, scale);
                const auto image = settle(canvas);
                branches(image, canvas);
                const auto builds = canvas.geometryBuildCount();
                require(paint(canvas) == image && paint(canvas) == image,
                        "repeated cached paints are identical without duplication");
                require(canvas.geometryBuildCount() == builds, "cached paints do not rebuild");
                if (width == 900 && scale == 9.) image.save("/tmp/rationalstudio-tangent-fixed.png");
            }
        }
        if (dpiSmoke) {
            require(canvas.devicePixelRatioF() > 1.0, "scaled Qt window");
            std::cout << "PASS: " << assertions << " high DPI assertions\n";
            return 0;
        }
        canvas.resize(900, 617);
        app.processEvents();
        canvas.setView(13.7, 0, .8);
        const auto before = settle(canvas);
        // Force a replacement to need many frames, then interrupt it again.
        require(canvas.setSamplingLimits({1024, 3072}), "small refresh slices");
        settle(canvas);
        const QPointF at(450, 308);
        for (int iteration = 0; iteration < 3; ++iteration) {
            QWheelEvent wheel(at, at, QPoint(), QPoint(0, -120), Qt::NoButton,
                              Qt::NoModifier, Qt::NoScrollPhase, false);
            QCoreApplication::sendEvent(&canvas, &wheel);
            const auto preview = paint(canvas);
            branches(preview, canvas); // overscan covers exposed areas
            waitForRefresh();
            const auto partial = paint(canvas);
            branches(partial, canvas); // old complete geometry survives yields
        }
        const auto final = settle(canvas);
        branches(final, canvas);
        require(paint(canvas) == final, "final navigation image is stable");
        require(before != final, "navigation changed displayed geometry");
        canvas.setView(canvas.centerX() + 8, canvas.centerY() + 3,
                       canvas.pixelsPerUnit() * .95);
        branches(paint(canvas), canvas); // toolbar zoom also retains complete geometry
        canvas.resize(940, 647);
        app.processEvents();
        branches(paint(canvas), canvas); // resizing uses the correctly transformed preview
        const auto resized = settle(canvas);
        branches(resized, canvas);
        require(canvas.setSamplingLimits({4, 4}), "minimal shared paint budget");
        require(paint(canvas) == resized && paint(canvas) == resized,
                "all curve previews survive when later curves get no callback budget");
        std::cout << "PASS: " << assertions << " canvas coverage assertions\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
