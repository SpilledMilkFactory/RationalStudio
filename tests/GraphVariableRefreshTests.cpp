#include "Expression.hpp"
#include "FunctionDefinitions.hpp"
#include "GraphCanvas.hpp"
#include "MainWindow.hpp"
#include "VariableSliderPanel.hpp"

#include <QApplication>
#include <QEventLoop>
#include <QImage>
#include <QLineEdit>
#include <QTabWidget>
#include <QTimer>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace rationalstudio;
namespace {
int assertions = 0;
void require(bool condition, const char* label) {
    ++assertions;
    if (!condition) throw std::runtime_error(label);
}
void render(GraphCanvas& canvas) {
    QImage image(canvas.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    canvas.render(&image);
    require(!image.isNull(), "valid rendered image");
}
void advanceEvents(int ms = 180) {
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}
GraphCurve curve(std::shared_ptr<const Expression> e, QColor color) {
    return {color, e, true, QStringLiteral("variable regression")};
}
}

int main(int argc, char** argv) {
    try {
        QApplication app(argc, argv);
        VariableScope scope;
        require(scope.declare("a"), "declare a");
        require(scope.declare("b"), "declare b");
        VariableBindings initial;
        require(initial.set("a", 2), "initial binding a");
        require(initial.set("b", 3), "initial binding b");
        auto first = std::make_shared<Expression>("a*x", scope);
        auto second = std::make_shared<Expression>("b*x", scope);
        auto independent = std::make_shared<Expression>("sin(x)", scope);
        FunctionDefinitions functions(scope);
        require(functions.define("f(t)=a*t+1").accepted, "defined capturing function");
        auto capture = std::make_shared<Expression>("f(x)", scope, functions.scope());
        require(first->valid() && second->valid() && independent->valid() && capture->valid(),
                "all expressions compile");

        GraphCanvas canvas;
        canvas.resize(650, 420);
        canvas.show();
        app.processEvents();
        canvas.setVariableBindings(initial, {"a", "b"});
        canvas.setCurves({curve(first, QColor("red")), curve(second, QColor("blue")),
                          curve(independent, QColor("green")), curve(capture, QColor("yellow"))});
        render(canvas);
        const auto initialBuilds = canvas.geometryBuildCount();
        require(initialBuilds == 4, "four initial geometries built");
        for (int i = 0; i < 3; ++i) render(canvas);
        require(canvas.geometryBuildCount() == initialBuilds,
                "ordinary repaints reuse geometry");

        VariableBindings changedA;
        require(changedA.set("a", 4) && changedA.set("b", 3), "changed a bindings");
        canvas.setVariableBindings(changedA, {"a"});
        render(canvas);
        require(canvas.geometryBuildCount() == initialBuilds + 2,
                "only direct and captured-a curves rebuild");
        auto count = canvas.geometryBuildCount();
        canvas.setVariableBindings(changedA, {});
        render(canvas);
        require(canvas.geometryBuildCount() == count, "no changes means no rebuild");
        canvas.setVariableBindings(changedA, {"not_referenced"});
        render(canvas);
        require(canvas.geometryBuildCount() == count, "unrelated change means no rebuild");
        VariableBindings changedB;
        require(changedB.set("a", 4) && changedB.set("b", 6), "changed b bindings");
        canvas.setVariableBindings(changedB, {"b"});
        render(canvas);
        require(canvas.geometryBuildCount() == count + 1,
                "only the b-dependent curve rebuilds");

        // End-to-end MainWindow contract: unknown identifier becomes valid
        // on slider creation, values update curves, and removal invalidates
        // only the referencing row. This catches a forgotten GUI wire-up.
        MainWindow window;
        window.resize(1250, 790);
        window.show();
        auto* tabs = window.findChild<QTabWidget*>();
        // Neither custom QWidget subclass currently declares Q_OBJECT, so
        // QObject::findChild<Subclass*>() would resolve QWidget's metaobject
        // and could incorrectly cast an unrelated child. Check RTTI instead.
        VariableSliderPanel* panel = nullptr;
        GraphCanvas* graph = nullptr;
        for (auto* widget : window.findChildren<QWidget*>()) {
            if (auto* candidate = dynamic_cast<VariableSliderPanel*>(widget)) panel = candidate;
            if (auto* candidate = dynamic_cast<GraphCanvas*>(widget)) graph = candidate;
        }
        require(tabs && panel && graph, "MainWindow contains graph and slider panel");
        tabs->setCurrentIndex(1);
        app.processEvents();
        advanceEvents();
        QLineEdit* expression = nullptr;
        for (auto* entry : window.findChildren<QLineEdit*>()) {
            if (entry->text() == QStringLiteral("y = sin(x)")) {
                expression = entry;
                break;
            }
        }
        require(expression != nullptr, "find editable graph row");
        expression->setText("y = a*x");
        advanceEvents();
        render(*graph); // unknown a does not create a drawable curve
        const auto beforeAdd = graph->geometryBuildCount();
        require(panel->addVariable("a", 2, -10, 10, 1), "create slider a");
        render(*graph);
        const auto afterAdd = graph->geometryBuildCount();
        require(afterAdd == beforeAdd + 1, "newly declared a enables invalid row");
        require(panel->setVariableValue("a", 3), "edit variable a");
        render(*graph);
        const auto afterChange = graph->geometryBuildCount();
        require(afterChange == afterAdd + 1, "edit a rebuilds one dependent curve");
        require(panel->addVariable("b", 2, -10, 10, 1), "create unrelated slider b");
        render(*graph);
        require(graph->geometryBuildCount() == afterChange,
                "unrelated declaration does not reparse or rebuild ready paths");
        require(panel->removeVariable("a"), "remove a");
        render(*graph);
        require(graph->geometryBuildCount() == afterChange,
                "removed name makes dependent row invalid, no extra geometry");
        require(panel->addVariable("a", 1), "re-add a");
        render(*graph);
        require(graph->geometryBuildCount() == afterChange + 1,
                "restored declaration repairs graph expression");
        std::cout << "PASS: " << assertions << " GraphVariableRefresh assertions\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
