#include "GraphCanvas.hpp"
#include "GraphWorkspacePersistence.hpp"
#include "MainWindow.hpp"
#include "MathExpressionEdit.hpp"
#include "PolarExpression.hpp"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QThread>
#include <QWidget>
#include <cmath>
#include <functional>
#include <iostream>

using namespace rationalstudio;
namespace {
int checks=0,failures=0;
void check(bool ok,const char* what) {
    ++checks;
    if(!ok) {++failures; std::cerr<<"FAIL: "<<what<<'\n';}
}
GraphCanvas* canvas(MainWindow& window) {
    for(auto* child:window.findChildren<QWidget*>())
        if(auto* c=dynamic_cast<GraphCanvas*>(child)) return c;
    return nullptr;
}
void render(GraphCanvas& graph) {
    for(int i=0;i<200;++i) {
        QApplication::processEvents();
        (void)graph.grab();
        if(!graph.samplingDiagnostics().lastPaintDeferredCurves) break;
    }
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    GraphWorkspaceState state;
    state.curves.push_back({"y = sin(x)",true,QColor("red")});
    WorkspaceCurve polar{"r = 2cos(θ)",true,QColor("blue")};
    polar.polar=true;
    polar.tMinimum=0;
    polar.tMaximum=6.283185307179586;
    state.curves.push_back(polar);
    state.sliders.push_back({"a",2,-10,10,1});
    const auto json=GraphWorkspacePersistence::encode(state);
    auto root=QJsonDocument::fromJson(json).object();
    check(root.value("version").toInt()==3,"polar requires v3 workspace");
    const auto encoded=root.value("expressions").toArray();
    check(encoded.size()==2,"mixed representation row count");
    check(encoded.size()==2 && encoded[1].toObject().value("kind").toString()=="polar",
          "polar kind persisted");
    GraphWorkspaceState roundtrip;
    QString error;
    check(GraphWorkspacePersistence::decode(json,roundtrip,&error),"mixed polar workspace decodes");
    check(roundtrip.curves.size()==2 && roundtrip.curves[1].polar,"polar row restored");
    check(roundtrip.curves.size()==2 && !roundtrip.curves[0].polar,"Cartesian preserved");
    check(roundtrip.curves.size()==2 && roundtrip.curves[1].expression==polar.expression,
          "Unicode polar radius text retained");
    check(roundtrip.curves.size()==2 && std::abs(roundtrip.curves[1].tMaximum-polar.tMaximum)<1e-12,
          "polar angular interval retained");
    auto mutate=[&](const char* label,const std::function<void(QJsonObject&)>& change) {
        auto changed=root;
        change(changed);
        GraphWorkspaceState untouched;
        check(!GraphWorkspacePersistence::decode(QJsonDocument(changed).toJson(),untouched,&error),label);
    };
    mutate("unknown polar kind rejected",[](QJsonObject& doc) {
        auto rows=doc.value("expressions").toArray();
        auto row=rows[1].toObject();row["kind"]="wrong";rows[1]=row;doc["expressions"]=rows;
    });
    mutate("invalid polar interval rejected",[](QJsonObject& doc) {
        auto rows=doc.value("expressions").toArray();
        auto row=rows[1].toObject();row["tMin"]=10;rows[1]=row;doc["expressions"]=rows;
    });
    mutate("unknown polar radius identifier rejected",[](QJsonObject& doc) {
        auto rows=doc.value("expressions").toArray();
        auto row=rows[1].toObject();row["expression"]="r = unknown(θ)";
        rows[1]=row;doc["expressions"]=rows;
    });
    mutate("bad label rejected",[](QJsonObject& doc) {
        auto rows=doc.value("expressions").toArray();
        auto row=rows[1].toObject();row["expression"]="x = 2cos(θ)";
        rows[1]=row;doc["expressions"]=rows;
    });
    // Cache isolation across polar and parametric representations.
    VariableScope local;
    check(local.declare("t"),"local parameter declared");
    const auto sources=polarCoordinateSources("r = 2cos(θ)");
    auto x=std::make_shared<Expression>(sources.x,local);
    auto y=std::make_shared<Expression>(sources.y,local);
    check(x->valid() && y->valid(),"polar Cartesian coordinates compile");
    GraphCanvas direct;
    direct.resize(640,420);
    direct.show();
    QApplication::processEvents();
    GraphCurve curve{QColor("blue"),x,true,"polar",y,0,6.283185307179586};
    curve.polarCurve=true;
    direct.setCurves({curve});
    render(direct);
    const auto count=direct.geometryBuildCount();
    check(count==1,"shared canvas renders polar geometry");
    curve.color=QColor("green");
    direct.setCurves({curve});
    render(direct);
    check(direct.geometryBuildCount()==count,"style change retains polar path cache");
    curve.tMax=3.141592653589793;
    direct.setCurves({curve});
    render(direct);
    check(direct.geometryBuildCount()==count+1,"angular interval invalidates polar path");
    curve.polarCurve=false;
    direct.setCurves({curve});
    render(direct);
    check(direct.geometryBuildCount()==count+2,"kind change cannot reuse polar path");
    QTemporaryDir temp;
    check(temp.isValid(),"temporary workspace directory");
    QFile file(temp.filePath("polar.json"));
    check(file.open(QIODevice::WriteOnly),"fixture opened");
    check(file.write(json)==json.size(),"fixture written");
    file.close();
    MainWindow window;
    window.resize(1250,800);
    window.show();
    if(auto* tabs=window.findChild<QTabWidget*>()) tabs->setCurrentIndex(1);
    QApplication::processEvents();
    check(window.loadWorkspace(file.fileName(),&error),"MainWindow loads polar workspace");
    auto* graph=canvas(window);
    check(graph!=nullptr,"GUI canvas accessible");
    MathExpressionEdit* polarInput=nullptr;
    for(auto* input:window.findChildren<MathExpressionEdit*>())
        if(input->objectName().startsWith("PolarRadius_")) polarInput=input;
    check(polarInput!=nullptr,"polar editor restored");
    if(polarInput) {
        check(polarInput->text()==polar.expression,"visible Unicode expression restored");
        check(polarInput->property("syntaxState").toString()=="valid","polar inline validation valid");
    }
    if(graph) {
        render(*graph);
        check(graph->geometryBuildCount()>=2,"polar and Cartesian both rendered");
    }
    if(polarInput && graph) {
        const auto initial=graph->geometryBuildCount();
        polarInput->setText("r = 3cos(θ)");
        for(int i=0;i<30;++i) {QApplication::processEvents();QThread::msleep(10);}
        render(*graph);
        check(graph->geometryBuildCount()>initial,"radius edit invalidates geometry");
        polarInput->setText("r = unknown(θ)");
        check(polarInput->property("syntaxState").toString()=="invalid","invalid radius inline feedback");
        polarInput->setText("r = 2cos(θ)");
        check(polarInput->property("syntaxState").toString()=="valid","corrected radius feedback recovers");
    }
    const auto out=temp.filePath("saved.json");
    check(window.saveWorkspace(out,&error),"MainWindow saves polar workspace");
    QFile saved(out);
    check(saved.open(QIODevice::ReadOnly),"saved workspace readable");
    GraphWorkspaceState savedState;
    check(GraphWorkspacePersistence::decode(saved.readAll(),savedState,&error),"saved polar JSON validates");
    check(savedState.curves.size()==2 && savedState.curves[1].polar,"save preserves polar row");
    std::cout<<(failures?"FAIL: ":"PASS: ")<<checks<<" PolarGraph assertions\n";
    return failures?1:0;
}
