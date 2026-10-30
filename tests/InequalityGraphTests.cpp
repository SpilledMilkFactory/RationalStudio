#include "GraphCanvas.hpp"
#include "GraphWorkspacePersistence.hpp"
#include "MainWindow.hpp"
#include "MathExpressionEdit.hpp"
#include "InequalityRegion.hpp"
#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QImage>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QToolButton>
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace rationalstudio;
namespace {
int tests = 0, failures = 0;
void check(bool passed, const char* why) {
    ++tests;
    if (!passed) { ++failures; std::cerr << "FAIL: " << why << '\n'; }
}
void pump(int ms = 25) {
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}
GraphCanvas* findCanvas(MainWindow& window) {
    for (auto* w : window.findChildren<QWidget*>())
        if (auto* c = dynamic_cast<GraphCanvas*>(w)) return c;
    return nullptr;
}
bool ready(GraphCanvas& canvas, int timeout = 3500) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeout) {
        canvas.repaint();
        QApplication::processEvents();
        if (canvas.visibleInequalityReady() && canvas.visibleGeometryReady()) return true;
        pump();
    }
    std::cerr << "DIAGNOSTIC: cached inequality not ready after " << timeout
              << "ms; sample evaluations=" << canvas.inequalityEvaluationCount()
              << " pending geometry=" << canvas.samplingDiagnostics().lastPaintDeferredCurves << '\n';
    return false;
}
QColor pixel(GraphCanvas& canvas, double fx, double fy) {
    QImage img(canvas.size(), QImage::Format_ARGB32);
    img.fill(Qt::black);
    canvas.render(&img);
    const int x = std::clamp(int(canvas.width() * fx), 0, canvas.width()-1);
    const int y = std::clamp(int(canvas.height() * fy), 0, canvas.height()-1);
    return img.pixelColor(x, y);
}
GraphCurve inequality(const char* body, bool above, bool inclusive) {
    GraphCurve curve{QColor("#fa675a"), std::make_shared<Expression>(body), true, "test"};
    curve.inequalityCurve = true;
    curve.inequalityAbove = above;
    curve.inequalityInclusive = inclusive;
    return curve;
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    {
        GraphCanvas canvas;
        canvas.resize(520, 340);
        canvas.setGridVisible(false);
        canvas.setAxesVisible(false);
        canvas.setCurves({inequality("0", true, false)});
        canvas.show();
        check(ready(canvas), "first inequality boundary and shade fully ready");
        auto above = pixel(canvas, 0.37, 0.23);
        auto below = pixel(canvas, 0.37, 0.77);
        check(above != below, "region above y=0 visibly shaded");
        check(below == QColor("#101722"), "region below y=0 remains background");
        const auto before = canvas.inequalityEvaluationCount();
        const auto builds = canvas.geometryBuildCount();
        canvas.repaint();
        canvas.repaint();
        check(canvas.inequalityEvaluationCount() == before, "unchanged shade reuses raster without callbacks");
        check(canvas.geometryBuildCount() == builds, "unchanged shade does not rebuild boundary");
        canvas.setCurves({inequality("0", false, true)});
        // New Expression pointer deliberately changes geometry identity.
        check(ready(canvas), "below inequality ready after representation change");
        check(pixel(canvas, .37, .23) == QColor("#101722"), "upper region unshaded for y<=0");
        check(pixel(canvas, .37, .77) != QColor("#101722"), "lower region shaded for y<=0");
        canvas.setCurves({inequality("1/x", true, false)});
        check(ready(canvas), "undefined boundary shading completes within work budget");
        canvas.setCurves({inequality("0", true, true)});
        check(ready(canvas), "inclusive boundary completes");
        // A scalar binding change must drop the shading raster even though
        // the expression's compiled AST identity remains the same.
        VariableScope scope;
        check(scope.declare("a"), "named variable scope accepted");
        GraphCurve dynamic = inequality("0", true, true);
        dynamic.function = std::make_shared<Expression>("a", scope);
        check(dynamic.function->valid(), "inequality compiles against named scalar");
        VariableBindings one;
        check(one.set("a", -2.), "first immutable scalar binding stored");
        canvas.setCurves({dynamic});
        canvas.setVariableBindings(one, {"a"});
        check(ready(canvas), "initial variable-driven shade completes");
        const auto boundBefore = canvas.inequalityEvaluationCount();
        VariableBindings two;
        check(two.set("a", 2.), "second scalar binding stored");
        canvas.setVariableBindings(two, {"a"});
        check(ready(canvas), "changed variable-driven shade completes");
        check(canvas.inequalityEvaluationCount() > boundBefore,
              "variable change invalidates cached shade");
        canvas.setCurves({inequality("0", true, true)});
        check(ready(canvas), "constant boundary restored");
        const auto count = canvas.inequalityEvaluationCount();
        canvas.setView(.25, 0, canvas.pixelsPerUnit());
        check(ready(canvas), "new viewport invalidates and rebuilds shade");
        check(canvas.inequalityEvaluationCount() > count, "viewport change recomputes region");
    }
    {
        MainWindow window;
        window.show();
        auto* tabs = window.findChild<QTabWidget*>();
        check(tabs != nullptr, "main tab control present");
        if (tabs) tabs->setCurrentIndex(1);
        pump(60);
        auto* add = window.findChild<QPushButton*>("addInequalityGraphRow");
        auto* canvas = findCanvas(window);
        check(add != nullptr, "inequality add control registered");
        check(canvas != nullptr, "shared GraphCanvas renderer retained");
        if (!add || !canvas) return 1;
        const auto initial = window.findChildren<MathExpressionEdit*>().size();
        add->click();
        auto fields = window.findChildren<MathExpressionEdit*>();
        check(fields.size() == initial + 1, "inequality add creates one row");
        MathExpressionEdit* edit = nullptr;
        for (auto* input : fields)
            if (input->objectName().startsWith("InequalityEquation_")) edit = input;
        check(edit != nullptr, "inequality row exposes persistent math editor");
        if (!edit) return 1;
        check(edit->text() == "y > x^2", "default expression is quadratic inequality");
        check(edit->property("syntaxState").toString() == "valid", "initial inequality is valid");
        check(ready(*canvas), "real MainWindow inequality graph completes");
        const auto previous = canvas->inequalityEvaluationCount();
        edit->setText("y <= x^2");
        pump(190);
        check(edit->property("syntaxState").toString() == "valid", "inclusive inequality accepted");
        check(ready(*canvas), "edited inequality recomputed");
        check(canvas->inequalityEvaluationCount() > previous, "inequality editing invalidates raster");
        edit->setText("y << x^2");
        pump(190);
        check(edit->property("syntaxState").toString() == "invalid", "invalid comparison is flagged");
        edit->setText("y >= x^2");
        pump(190);
        check(edit->property("syntaxState").toString() == "valid", "row repaired successfully");
        QTemporaryDir temp;
        check(temp.isValid(), "temporary workspace directory ready");
        if (temp.isValid()) {
            const QString path = temp.filePath("inequality.rsg");
            QString error;
            check(window.saveWorkspace(path, &error), "workspace save succeeds");
            GraphWorkspaceState state;
            QFile file(path);
            if (file.open(QIODevice::ReadOnly)) {
                const QByteArray original = file.readAll();
                check(GraphWorkspacePersistence::decode(original, state, &error),
                      "workspace v3 decodes inequality");
                auto root = QJsonDocument::fromJson(original).object();
                auto entries = root.value("expressions").toArray();
                if (!entries.isEmpty()) {
                    auto corrupt = entries.at(entries.size()-1).toObject();
                    corrupt["kind"] = "inequality";
                    corrupt["expression"] = "y > unknown_scalar";
                    entries[entries.size()-1] = corrupt;
                    root["expressions"] = entries;
                    GraphWorkspaceState rejected;
                    check(!GraphWorkspacePersistence::decode(
                              QJsonDocument(root).toJson(), rejected, &error),
                          "unresolved inequality workspace is rejected transactionally");
                    corrupt["expression"] = "y > x > 2";
                    entries[entries.size()-1] = corrupt;
                    root["expressions"] = entries;
                    check(!GraphWorkspacePersistence::decode(
                              QJsonDocument(root).toJson(), rejected, &error),
                          "invalid inequality comparator rejected on load");
                }
                check(std::any_of(state.curves.begin(), state.curves.end(),
                    [](const WorkspaceCurve& row) { return row.inequality && row.expression == "y >= x^2"; }),
                      "workspace inequality and comparator are preserved");
            } else check(false, "workspace file readable");
            MainWindow restored;
            check(restored.loadWorkspace(path, &error), "MainWindow inequality workspace restores");
            bool found = false;
            for (auto* field : restored.findChildren<MathExpressionEdit*>())
                found |= field->text() == "y >= x^2";
            check(found, "restored inequality text retained");
        }
    }
    std::cout << (failures ? "FAIL: " : "PASS: ") << tests << " InequalityGraph assertions\n";
    return failures ? 1 : 0;
}
