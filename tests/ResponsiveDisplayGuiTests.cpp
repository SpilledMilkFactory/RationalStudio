#include "MainWindow.hpp"
#include "GraphCanvas.hpp"

#include <QApplication>
#include <QElapsedTimer>
#include <QImage>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QTabBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QWidget>

#include <cmath>
#include <iostream>
#include <memory>

using namespace rationalstudio;
namespace {
int assertions = 0;
int failures = 0;
void check(bool condition, const char* description) {
    ++assertions;
    if (!condition) { ++failures; std::cerr << "FAIL: " << description << '\n'; }
}
GraphCanvas* graphIn(QWidget& window) {
    // Qt 6 typed findChild<Derived*> requires Q_OBJECT. GraphCanvas does not
    // declare it, so inspect QWidget children with checked C++ RTTI.
    for (auto* child : window.findChildren<QWidget*>())
        if (auto* canvas = dynamic_cast<GraphCanvas*>(child)) return canvas;
    return nullptr;
}
bool ready(GraphCanvas& canvas) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 5000) {
        canvas.repaint();
        QApplication::processEvents();
        if (canvas.visibleGeometryReady() && canvas.visibleInequalityReady())
            return true;
    }
    return false;
}
QColor sampled(const QImage& image, double x, double y) {
    return image.pixelColor(int(x * image.width()), int(y * image.height()));
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    const bool scaled = qEnvironmentVariable("QT_SCALE_FACTOR").toDouble() >= 1.5;
    QTemporaryDir isolated;
    check(isolated.isValid(), "temporary settings directory available");
    if (!isolated.isValid()) return 2;
    MainWindow window(nullptr, isolated.filePath("responsive.ini"));
    check(window.minimumWidth() <= 760 && window.minimumHeight() <= 540,
          "window supports compact logical dimensions");
    window.show();
    auto* tabs = window.findChild<QTabWidget*>();
    check(tabs && tabs->count() == 4, "four existing calculator tabs retained");
    auto* scientificScroll = window.findChild<QScrollArea*>("scientificPageScroll");
    check(scientificScroll && scientificScroll->widgetResizable(),
          "scientific content can scroll at reduced height");
    if (!tabs) return 2;
    for (const auto dimensions : {QSize(760, 540), QSize(920, 620), QSize(1280, 800)}) {
        window.resize(dimensions);
        QApplication::processEvents();
        check(window.width() >= 760 && window.height() >= 540,
              "resized window respects supported minimum");
        if (dimensions.width() == 760)
            check(window.width() <= 840 && window.height() <= 610,
                  "compact request is not forced back to the former 1000x650 minimum");
        tabs->setCurrentIndex(0);
        QApplication::processEvents();
        check(scientificScroll && scientificScroll->isVisibleTo(&window),
              "scientific scroll viewport accessible after resize");
        tabs->setCurrentIndex(1);
        QApplication::processEvents();
        auto* graph = graphIn(window);
        check(graph && graph->width() >= 260 && graph->height() >= 210,
              "graph viewport retains positive usable logical dimensions");
        auto* features = window.findChild<QPushButton*>("graphFindFeatures");
        auto* intersections = window.findChild<QPushButton*>("graphFindIntersections");
        check(features && features->isVisibleTo(&window) &&
              intersections && intersections->isVisibleTo(&window),
              "analysis actions remain exposed after resize");
        tabs->setCurrentIndex(3);
        QApplication::processEvents();
        auto* advanced = window.findChild<QTabWidget*>("advancedMathTabs");
        check(advanced && advanced->count() == 6 &&
              advanced->tabBar()->usesScrollButtons(),
              "advanced operation tabs remain navigable at compact widths");
    }
    window.hide();
    QApplication::processEvents();

    GraphCanvas canvas;
    canvas.resize(520, 340);
    canvas.setGridVisible(false);
    canvas.setAxesVisible(false);
    GraphCurve region{QColor("#fa675a"), std::make_shared<Expression>("0"), true, "y > 0"};
    region.inequalityCurve = true;
    region.inequalityAbove = true;
    region.inequalityInclusive = false;
    canvas.setCurves({region});
    canvas.show();
    QApplication::processEvents();
    check(ready(canvas), "high DPI inequality shade and curve geometry completed");
    QImage image = canvas.grab().toImage();
    check(!image.isNull(), "graph screenshot produces a nonempty raster");
    if (!image.isNull()) {
        const double dpr = image.devicePixelRatioF();
        if (scaled)
            check(dpr >= 1.5, "scaled Qt paint device reports high DPI raster");
        else
            check(dpr >= 1.0, "normal Qt paint device reports valid pixel ratio");
        check(std::abs(image.width() / dpr - canvas.width()) <= 1.0,
              "physical width maps to logical widget width");
        check(std::abs(image.height() / dpr - canvas.height()) <= 1.0,
              "physical height maps to logical widget height");
        check(sampled(image, 0.37, 0.23) != sampled(image, 0.37, 0.77),
              "inequality shade remains visible above its boundary");
    }
    const auto sampleCount = canvas.inequalityEvaluationCount();
    const auto geometryCount = canvas.geometryBuildCount();
    canvas.repaint();
    canvas.repaint();
    check(canvas.inequalityEvaluationCount() == sampleCount,
          "repeat painting reuses finished shaded raster at current DPR");
    check(canvas.geometryBuildCount() == geometryCount,
          "repeat painting reuses finished curve geometry");
    canvas.resize(610, 400);
    QApplication::processEvents();
    check(ready(canvas), "resize rebuilds geometry and shading for new logical size");
    check(canvas.geometryBuildCount() > geometryCount,
          "logical resize causes updated curve geometry");
    std::cout << (failures ? "FAIL: " : "PASS: ") << assertions
              << " ResponsiveDisplayGui assertions" << (scaled ? " (2x DPI)" : " (1x DPI)") << '\n';
    return failures ? 1 : 0;
}
