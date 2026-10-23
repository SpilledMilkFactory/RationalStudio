#include "GraphCanvas.hpp"

#include <QApplication>
#include <QImage>
#include <QString>
#include <QWidget>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace rationalstudio;

namespace {
int assertions = 0;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
    ++assertions;
}
void repaintNow(GraphCanvas& widget) {
    // Render the actual widget through its paintEvent, not an isolated
    // cache mock. Qt's offscreen platform avoids needing an X11 display.
    QImage output(widget.size(), QImage::Format_ARGB32_Premultiplied);
    output.fill(Qt::transparent);
    widget.render(&output);
    require(!output.isNull(), "offscreen widget image exists");
}
std::shared_ptr<const Expression> expr(const char* source) {
    auto result = std::make_shared<Expression>(source);
    require(result->valid(), "expression parses");
    return result;
}
GraphCurve curve(std::shared_ptr<const Expression> expression, bool visible,
                 QColor color, const char* label) {
    return GraphCurve{color, std::move(expression), visible, QString::fromUtf8(label)};
}
} // namespace

int main(int argc, char** argv) {
    try {
        QApplication application(argc, argv);
        GraphCanvas canvas;
        canvas.resize(800, 600);
        canvas.show();
        application.processEvents();

        const auto a = expr("sin(x)");
        const auto b = expr("x^2");
        const auto c = expr("sin(x)");
        canvas.setCurves({curve(a, true, QColor("red"), "sin(x)"),
                          curve(b, true, QColor("blue"), "x^2")});
        repaintNow(canvas);
        const auto first = canvas.geometryBuildCount();
        require(first >= 2, "two functions built at least once");
        for (int i = 0; i < 8; ++i) repaintNow(canvas);
        require(canvas.geometryBuildCount() == first,
                "repaints and pointer-inspection overlay do not resample");

        canvas.setGridVisible(false);
        canvas.setAxesVisible(false);
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == first,
                "grid and axes style changes do not resample");

        canvas.setCurves({curve(a, true, QColor("green"), "f(x) = sin(x)"),
                          curve(b, true, QColor("red"), "y = x^2")});
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == first,
                "color and labels do not resample same functions");

        canvas.setCurves({curve(b, true, QColor("red"), "x^2"),
                          curve(a, true, QColor("green"), "sin(x)")});
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == first,
                "row reorder does not resample");

        canvas.setCurves({curve(b, false, QColor("red"), "x^2"),
                          curve(a, true, QColor("green"), "sin(x)")});
        repaintNow(canvas);
        canvas.setCurves({curve(b, true, QColor("red"), "x^2"),
                          curve(a, true, QColor("green"), "sin(x)")});
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == first,
                "visibility toggle preserves built paths");

        // Changing only one compiled expression rebuilds only that curve.
        canvas.setCurves({curve(b, true, QColor("red"), "x^2"),
                          curve(c, true, QColor("green"), "sin(x)")});
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == first + 1,
                "replaced expression identity rebuilds only one curve");

        auto count = canvas.geometryBuildCount();
        canvas.setAngleMode(AngleMode::Radians);
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == count,
                "unchanged angle does not rebuild");
        canvas.setAngleMode(AngleMode::Degrees);
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == count + 2,
                "new angle rebuilds both visible curves");

        count = canvas.geometryBuildCount();
        canvas.setLastAnswer(0.0);
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == count,
                "unchanged ans does not rebuild");
        canvas.setLastAnswer(3.5);
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == count + 2,
                "changed ans rebuilds geometry");

        count = canvas.geometryBuildCount();
        canvas.setView(canvas.centerX(), canvas.centerY(), canvas.pixelsPerUnit());
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == count,
                "unchanged view does not rebuild");
        canvas.setView(0.5, -0.25, canvas.pixelsPerUnit());
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == count + 2,
                "changed viewport rebuilds both visible curves");

        count = canvas.geometryBuildCount();
        canvas.setView(0, 0, -1);
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == count,
                "invalid viewport request does not rebuild");

        count = canvas.geometryBuildCount();
        canvas.resize(910, 670);
        application.processEvents();
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == count + 2,
                "resize invalidates and rebuilds cached geometry");

        count = canvas.geometryBuildCount();
        canvas.setCurves({curve(b, false, QColor("red"), "x^2"),
                          curve(c, true, QColor("green"), "sin(x)")});
        canvas.setView(-1, 2, 80);
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == count + 1,
                "hidden curve stays lazy after viewport change");
        canvas.setCurves({curve(b, true, QColor("red"), "x^2"),
                          curve(c, true, QColor("green"), "sin(x)")});
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == count + 2,
                "re-enabled hidden curve rebuilds stale geometry once");

        count = canvas.geometryBuildCount();
        canvas.setCurves({});
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == count,
                "empty curve list never samples");
        canvas.setCurves({curve(a, true, QColor("red"), "sin(x)")});
        repaintNow(canvas);
        require(canvas.geometryBuildCount() == count + 1,
                "removed and re-added curve must rebuild");

        std::cout << "PASS: " << assertions << " GraphCanvasCache assertions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
