#include "GraphCanvas.hpp"
#include "GraphWorkspacePersistence.hpp"
#include "MainWindow.hpp"
#include "MathExpressionEdit.hpp"
#include "ImplicitContour.hpp"

#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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
void check(bool ok,const char* label) {
    ++checks;
    if(!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
GraphCanvas* findCanvas(MainWindow& window) {
    for(auto* widget:window.findChildren<QWidget*>())
        if(auto* canvas=dynamic_cast<GraphCanvas*>(widget)) return canvas;
    return nullptr;
}
void render(GraphCanvas& canvas) {
    for(int i=0;i<350;++i) {
        QApplication::processEvents();
        (void)canvas.grab();
        if(!canvas.samplingDiagnostics().lastPaintDeferredCurves) break;
    }
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    GraphWorkspaceState state;
    state.curves.push_back({"y = sin(x)",true,QColor("red")});
    WorkspaceCurve implicit{"x²+y²=25",true,QColor("blue")};
    implicit.implicit=true;
    state.curves.push_back(implicit);
    state.sliders.push_back({"a",2,-10,10,1});
    const auto json=GraphWorkspacePersistence::encode(state);
    const auto root=QJsonDocument::fromJson(json).object();
    check(root.value("version").toInt()==3,"implicit workspace uses v3");
    auto rows=root.value("expressions").toArray();
    check(rows.size()==2,"two representation rows saved");
    check(rows[1].toObject().value("kind").toString()=="implicit","implicit kind saved");
    GraphWorkspaceState decoded;
    QString error;
    check(GraphWorkspacePersistence::decode(json,decoded,&error),"mixed workspace decoded");
    check(decoded.curves.size()==2&&decoded.curves[1].implicit,"implicit kind restored");
    check(decoded.curves.size()==2&&!decoded.curves[0].implicit,"Cartesian row preserved");
    check(decoded.curves.size()==2&&decoded.curves[1].expression==implicit.expression,
          "Unicode source roundtrips");
    const auto reject=[&](const char* name,const std::function<void(QJsonObject&)>& change) {
        auto modified=root;
        change(modified);
        GraphWorkspaceState untouched;
        check(!GraphWorkspacePersistence::decode(QJsonDocument(modified).toJson(),untouched,&error),name);
    };
    reject("unknown representation rejected",[](QJsonObject& doc) {
        auto items=doc.value("expressions").toArray();
        auto row=items[1].toObject(); row["kind"]="unknown"; items[1]=row; doc["expressions"]=items;
    });
    reject("missing right side rejected",[](QJsonObject& doc) {
        auto items=doc.value("expressions").toArray();
        auto row=items[1].toObject(); row["expression"]="x^2+y^2="; items[1]=row; doc["expressions"]=items;
    });
    reject("unbound identifier rejected",[](QJsonObject& doc) {
        auto items=doc.value("expressions").toArray();
        auto row=items[1].toObject(); row["expression"]="unknown+y=2"; items[1]=row; doc["expressions"]=items;
    });
    reject("incorrect function rejected",[](QJsonObject& doc) {
        auto items=doc.value("expressions").toArray();
        auto row=items[1].toObject(); row["expression"]="sin()=y"; items[1]=row; doc["expressions"]=items;
    });
    // Cache stability for presentation-only edits and representation switches.
    VariableScope local;
    check(local.declare("y"),"local y declared");
    auto source=implicitEquationSource("x^2+y^2=25");
    auto field=std::make_shared<Expression>(source.expression,local);
    check(field->valid(),"implicit scalar field compiles");
    GraphCanvas direct;
    direct.resize(640,420);
    direct.show();
    QApplication::processEvents();
    GraphCurve curve{QColor("blue"),field,true,"implicit circle"};
    curve.implicitCurve=true;
    direct.setCurves({curve});
    render(direct);
    auto built=direct.geometryBuildCount();
    check(built==1,"shared renderer builds contour geometry");
    curve.color=QColor("green");
    direct.setCurves({curve});
    render(direct);
    check(direct.geometryBuildCount()==built,"style change retains contour cache");
    curve.implicitCurve=false;
    direct.setCurves({curve});
    render(direct);
    check(direct.geometryBuildCount()==built+1,"representation switch invalidates path");
    curve.implicitCurve=true;
    direct.setCurves({curve});
    render(direct);
    check(direct.geometryBuildCount()==built+2,"switching back invalidates Cartesian path");
    QTemporaryDir temp;
    check(temp.isValid(),"workspace temp directory valid");
    QFile file(temp.filePath("implicit.json"));
    check(file.open(QIODevice::WriteOnly),"workspace fixture opened");
    check(file.write(json)==json.size(),"fixture written");
    file.close();
    MainWindow window;
    window.resize(1250,800);
    window.show();
    if(auto* tabs=window.findChild<QTabWidget*>()) tabs->setCurrentIndex(1);
    QApplication::processEvents();
    check(window.loadWorkspace(file.fileName(),&error),"MainWindow restores implicit workspace");
    auto* graph=findCanvas(window);
    check(graph!=nullptr,"actual GraphCanvas found");
    MathExpressionEdit* input=nullptr;
    for(auto* field:window.findChildren<MathExpressionEdit*>())
        if(field->objectName().startsWith("ImplicitEquation_")) input=field;
    check(input!=nullptr,"implicit editor restored");
    if(input) {
        check(input->text()==implicit.expression,"visible equation preserved");
        check(input->property("syntaxState").toString()=="valid","inline validation valid");
    }
    if(graph) {
        render(*graph);
        check(graph->geometryBuildCount()>=2,"Cartesian and implicit both render");
    }
    if(input&&graph) {
        const auto previous=graph->geometryBuildCount();
        input->setText("x²+y²=9");
        for(int i=0;i<35;++i){QApplication::processEvents();QThread::msleep(10);}
        render(*graph);
        check(graph->geometryBuildCount()>previous,"equation edit triggers new geometry");
        input->setText("x²+y²=");
        check(input->property("syntaxState").toString()=="invalid","invalid equation reported");
        input->setText("x²+y²=25");
        check(input->property("syntaxState").toString()=="valid","corrected equation recovers");
    }
    const auto out=temp.filePath("saved.json");
    check(window.saveWorkspace(out,&error),"MainWindow saves implicit workspace");
    QFile saved(out);
    check(saved.open(QIODevice::ReadOnly),"saved JSON readable");
    GraphWorkspaceState restored;
    check(GraphWorkspacePersistence::decode(saved.readAll(),restored,&error),"saved workspace decodes");
    check(restored.curves.size()==2 && restored.curves[1].implicit,"save retains implicit kind");
    const auto previousBytes=QJsonDocument(root).toJson();
    Q_UNUSED(previousBytes);
    std::cout<<(failures?"FAIL: ":"PASS: ")<<checks<<" ImplicitGraph assertions\n";
    return failures?1:0;
}
