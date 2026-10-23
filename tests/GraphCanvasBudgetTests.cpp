#include "GraphCanvas.hpp"
#include <QApplication>
#include <QImage>
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace rationalstudio;
namespace {
int assertions=0;
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
    ++assertions;
}
void paint(GraphCanvas& canvas) {
    QImage image(canvas.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    canvas.render(&image);
    check(!image.isNull(), "valid offscreen image");
}
GraphCurve curve(const char* source, QColor color) {
    auto f = std::make_shared<Expression>(source);
    check(f->valid(), "expression parses");
    return GraphCurve{color, f, true, QString::fromUtf8(source)};
}
}
int main(int argc,char** argv) {
    try {
        QApplication application(argc,argv);
        GraphCanvas canvas;
        canvas.resize(800,600);
        canvas.show();
        application.processEvents();
        const auto defaultLimits = canvas.samplingLimits();
        check(defaultLimits.maxEvaluationsPerPaint >= 4 &&
              defaultLimits.maxEvaluationsPerCurve >= 4, "valid default limits");
        check(!canvas.setSamplingLimits({3,100}) &&
              !canvas.setSamplingLimits({100,0}), "reject invalid limits");
        check(canvas.samplingLimits().maxEvaluationsPerPaint ==
              defaultLimits.maxEvaluationsPerPaint, "reject settings transactionally");
        canvas.setCurves({curve("sin(x*75)", QColor("red")),
                          curve("cos(x*99)", QColor("blue")),
                          curve("tanh(1000*x)", QColor("green"))});
        check(canvas.setSamplingLimits({8,8}), "accept small deterministic budget");
        paint(canvas);
        check(canvas.samplingDiagnostics().lastPaintEvaluations <= 8,
              "one paint never exceeds shared callback budget");
        check(canvas.samplingDiagnostics().truncatedCurveBuilds >= 1,
              "per-curve truncation is visible in diagnostics");
        for (int i=0;i<12 && canvas.geometryBuildCount()<3;++i) {
            application.processEvents();
            paint(canvas);
            check(canvas.samplingDiagnostics().lastPaintEvaluations <= 8,
                  "every paint respects cap");
        }
        check(canvas.geometryBuildCount() == 3,
              "deferred dirty curves eventually receive budget");
        const auto count = canvas.geometryBuildCount();
        const auto total = canvas.samplingDiagnostics().totalEvaluations;
        paint(canvas);
        check(canvas.geometryBuildCount() == count, "cached paths not resampled");
        check(canvas.samplingDiagnostics().totalEvaluations == total,
              "no callback on unchanged repaint");
        check(canvas.setSamplingLimits({8192,32768}), "larger limit accepted");
        paint(canvas);
        check(canvas.geometryBuildCount() == count+3,
              "raising limits rebuilds partial cached paths");
        check(canvas.samplingDiagnostics().lastPaintEvaluations <= 32768,
              "large budget still enforced");
        std::cout << "PASS: " << assertions << " GraphCanvasBudget assertions\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
