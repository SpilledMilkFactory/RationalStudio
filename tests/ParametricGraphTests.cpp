#include "GraphCanvas.hpp"
#include "GraphWorkspacePersistence.hpp"
#include "MainWindow.hpp"
#include "MathExpressionEdit.hpp"
#include "VariableSliderPanel.hpp"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTabWidget>
#include <QWidget>
#include <QThread>
#include <cmath>
#include <iostream>

using namespace rationalstudio;
namespace {
int checks=0, failures=0;
void check(bool yes, const char* what) {
    ++checks;
    if(!yes) { ++failures; std::cerr << "FAIL: " << what << '\n'; }
}
GraphCanvas* canvas(MainWindow& window) {
    for (auto* child : window.findChildren<QWidget*>())
        if (auto* c=dynamic_cast<GraphCanvas*>(child)) return c;
    return nullptr;
}
void render(GraphCanvas& graph) {
    for (int i=0;i<200;++i) {
        QApplication::processEvents();
        (void)graph.grab();
        if (!graph.samplingDiagnostics().lastPaintDeferredCurves) break;
    }
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    GraphWorkspaceState state;
    state.curves.push_back({"y = x^2", true, QColor("red")});
    WorkspaceCurve parametric{"x(t) = cos(t)", true, QColor("blue")};
    parametric.parametric=true;
    parametric.yExpression="y(t) = sin(t)";
    parametric.tMinimum=0;
    parametric.tMaximum=6.283185307179586;
    state.curves.push_back(parametric);
    state.sliders.push_back({"a", 2, -10, 10, 1});
    const QByteArray json = GraphWorkspacePersistence::encode(state);
    auto root = QJsonDocument::fromJson(json).object();
    check(root.value("version").toInt()==3, "parametric workspace uses version 3");
    GraphWorkspaceState roundtrip;
    QString error;
    check(GraphWorkspacePersistence::decode(json, roundtrip, &error), "version 3 parses");
    check(roundtrip.curves.size()==2, "both graph kinds restored");
    check(roundtrip.curves.size()==2 && !roundtrip.curves[0].parametric, "Cartesian row retained");
    check(roundtrip.curves.size()==2 && roundtrip.curves[1].parametric, "parametric row retained");
    check(roundtrip.curves.size()==2 && roundtrip.curves[1].yExpression==parametric.yExpression,
          "Y coordinate retained");
    check(roundtrip.curves.size()==2 && std::abs(roundtrip.curves[1].tMaximum-6.283185307179586)<1e-10,
          "interval retained");
    auto mutate = [&](const char* title, const std::function<void(QJsonObject&)>& action) {
        auto modified=root;
        action(modified);
        GraphWorkspaceState unchanged;
        check(!GraphWorkspacePersistence::decode(QJsonDocument(modified).toJson(), unchanged, &error), title);
    };
    mutate("unknown kind rejected", [](QJsonObject& obj){
        auto curves=obj.value("expressions").toArray();
        auto item=curves[1].toObject(); item["kind"]="something-else";
        curves[1]=item; obj["expressions"]=curves;
    });
    mutate("reversed interval rejected", [](QJsonObject& obj){
        auto curves=obj.value("expressions").toArray();
        auto item=curves[1].toObject(); item["tMin"]=10;
        curves[1]=item; obj["expressions"]=curves;
    });
    mutate("invalid Y function rejected", [](QJsonObject& obj){
        auto curves=obj.value("expressions").toArray();
        auto item=curves[1].toObject(); item["yExpression"]="y(t) = unknown(t)";
        curves[1]=item; obj["expressions"]=curves;
    });
    // Renderer-level contract: Y-expression and interval changes must
    // invalidate the cached parametric curve even when X is unchanged.
    VariableScope local;
    check(local.declare("t"), "local t declaration");
    auto coordX = std::make_shared<Expression>("cos(t)", local);
    auto coordY = std::make_shared<Expression>("sin(t)", local);
    auto coordY2 = std::make_shared<Expression>("sin(t)+0.2", local);
    check(coordX->valid() && coordY->valid() && coordY2->valid(),
          "parametric coordinates compile");
    GraphCanvas direct;
    direct.resize(640,420);
    direct.show();
    app.processEvents();
    direct.setCurves({{QColor("red"), coordX, true, "circle", coordY, 0, 6.283185307179586}});
    render(direct);
    const auto initial = direct.geometryBuildCount();
    check(initial==1, "one parametric path generated");
    direct.setCurves({{QColor("blue"), coordX, true, "same coordinates", coordY, 0, 6.283185307179586}});
    render(direct);
    check(direct.geometryBuildCount()==initial, "color and label reuse path");
    direct.setCurves({{QColor("blue"), coordX, true, "edited Y", coordY2, 0, 6.283185307179586}});
    render(direct);
    check(direct.geometryBuildCount()==initial+1, "Y coordinate invalidates path");
    direct.setCurves({{QColor("blue"), coordX, true, "half range", coordY2, 0, 3.141592653589793}});
    render(direct);
    check(direct.geometryBuildCount()==initial+2, "interval invalidates path");
    // Older all-Cartesian workspaces stay on the version-2 format.
    state.curves.pop_back();
    check(QJsonDocument::fromJson(GraphWorkspacePersistence::encode(state))
          .object().value("version").toInt()==2, "Cartesian-only remains v2");
    QTemporaryDir temp;
    check(temp.isValid(), "temp folder");
    QFile file(temp.filePath("parametric.json"));
    check(file.open(QIODevice::WriteOnly), "fixture opened");
    check(file.write(json)==json.size(), "fixture written"); file.close();
    MainWindow window;
    window.resize(1250,800);
    window.show();
    if (auto* tabs=window.findChild<QTabWidget*>()) tabs->setCurrentIndex(1);
    app.processEvents();
    check(window.loadWorkspace(file.fileName(), &error), "real GUI loads v3");
    auto* graph=canvas(window);
    check(graph!=nullptr, "GraphCanvas located");
    MathExpressionEdit* x=nullptr;
    MathExpressionEdit* y=nullptr;
    for (auto* editor : window.findChildren<MathExpressionEdit*>()) {
        if (editor->objectName().startsWith("ParametricX_")) x=editor;
        if (editor->objectName().startsWith("ParametricY_")) y=editor;
    }
    check(x && y, "both parametric coordinate editors created");
    if (x && y) {
        check(x->text()==parametric.expression && y->text()==parametric.yExpression,
              "parametric GUI source preserved");
        check(x->property("syntaxState").toString()=="valid", "valid x feedback");
        check(y->property("syntaxState").toString()=="valid", "valid y feedback");
    }
    if (graph) {
        render(*graph);
        check(graph->geometryBuildCount()>=2, "parametric and Cartesian geometry built");
    }
    if (y && graph) {
        const auto baseline=graph->geometryBuildCount();
        y->setText("y(t) = sin(t)+0.2");
        // Existing graph update debounce is intentionally not bypassed.
        for (int i=0;i<30;++i) { QApplication::processEvents(); QThread::msleep(10); }
        render(*graph);
        check(graph->geometryBuildCount()>baseline, "Y edit rebuilds parametric curve");
    }
    const auto out=temp.filePath("roundtrip.json");
    check(window.saveWorkspace(out,&error), "GUI saves parametric workspace");
    QFile saved(out); check(saved.open(QIODevice::ReadOnly), "saved JSON readable");
    GraphWorkspaceState savedState;
    check(GraphWorkspacePersistence::decode(saved.readAll(),savedState,&error), "saved v3 valid");
    check(savedState.curves.size()==2 && savedState.curves[1].parametric,
          "saved v3 retains parametric row");
    std::cout << (failures?"FAIL: ":"PASS: ") << checks
              << " ParametricGraph assertions\n";
    return failures?1:0;
}
