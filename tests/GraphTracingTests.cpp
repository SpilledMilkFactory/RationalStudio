#include "GraphCanvas.hpp"
#include <QApplication>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QMouseEvent>
#include <QThread>
#include <cmath>
#include <iostream>
#include <memory>

using namespace rationalstudio;
static int checks=0, failures=0;
void expect(bool ok, const char* message) {
    ++checks;
    if (!ok) {++failures; std::cerr<<"FAIL: "<<message<<'\n';}
}
static constexpr double pi=3.14159265358979323846;
static void settle(GraphCanvas& canvas) {
    QElapsedTimer timer; timer.start();
    while (!canvas.visibleGeometryReady() && timer.elapsed()<9000) {
        canvas.repaint();
        QCoreApplication::processEvents(QEventLoop::AllEvents, 25);
        QThread::msleep(3);
    }
    canvas.repaint();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 25);
}
static QPointF location(const GraphCanvas& canvas, double x, double y) {
    return {canvas.width()*0.5 + (x-canvas.centerX())*canvas.pixelsPerUnit(),
            canvas.height()*0.5 - (y-canvas.centerY())*canvas.pixelsPerUnit()};
}
static void hover(GraphCanvas& canvas, double x, double y) {
    const QPointF pos=location(canvas,x,y);
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QMouseEvent event(QEvent::MouseMove,pos,pos,Qt::NoButton,Qt::NoButton,Qt::NoModifier);
#else
    QMouseEvent event(QEvent::MouseMove,pos,Qt::NoButton,Qt::NoButton,Qt::NoModifier);
#endif
    QApplication::sendEvent(&canvas,&event);
    QCoreApplication::processEvents(QEventLoop::AllEvents,10);
}
int main(int argc,char**argv) {
    QApplication app(argc,argv);
    GraphCanvas canvas;
    canvas.resize(800,600);
    canvas.setView(0,0,70);
    canvas.show();
    QString label;
    canvas.onPointerInfo=[&label](const QString& s){label=s;};
    {
        canvas.setCurves({{QColor("#ff0000"),std::make_shared<Expression>("x^2"),true,"y=x^2"}});
        settle(canvas);
        expect(canvas.visibleGeometryReady(),"Cartesian ready");
        const auto count=canvas.geometryBuildCount();
        hover(canvas,1,1);
        expect(label.contains("Cartesian"),"Cartesian identified");
        expect(label.contains("(x,y)"),"Cartesian coordinates reported");
        expect(canvas.geometryBuildCount()==count,"tracing does not rebuild geometry");
        canvas.setSignificantPoints({{1,1,"Row 1: root",QColor("#ff0000")}});
        hover(canvas,1,1);
        expect(label.contains("root") && label.contains("approximate"),"significant point inspectable");
        canvas.setSignificantPoints({});
        hover(canvas,1,1);
        expect(!label.contains("root"),"feature point cleared");
    }
    VariableScope scope;
    expect(scope.declare("t"),"parameter declared");
    {
        GraphCurve curve{QColor("#00ff00"),std::make_shared<Expression>("cos(t)",scope),true,"circle",
                         std::make_shared<Expression>("sin(t)",scope),0,2*pi};
        canvas.setCurves({curve});
        settle(canvas);
        expect(canvas.visibleGeometryReady(),"parametric ready");
        hover(canvas,0,1);
        expect(label.contains("Parametric"),"parametric identified");
        expect(label.contains("t="),"parametric parameter displayed");
        expect(label.contains("(x,y)"),"parametric Cartesian coordinates displayed");
        const auto builds=canvas.geometryBuildCount();
        canvas.repaint();
        hover(canvas,0,1);
        expect(canvas.geometryBuildCount()==builds,"hover reuses cached path");
    }
    {
        GraphCurve curve{QColor("#00ffff"),std::make_shared<Expression>("(-2)*cos(t)",scope),true,"negative radius",
                         std::make_shared<Expression>("(-2)*sin(t)",scope),0,2*pi};
        curve.polarCurve=true;
        canvas.setCurves({curve});
        settle(canvas);
        expect(canvas.visibleGeometryReady(),"polar ready");
        hover(canvas,-2,0);
        expect(label.contains("Polar"),"polar identified");
        expect(label.contains("θ="),"polar angle shown");
        expect(label.contains("r=-2"),"signed negative polar radius preserved");
        expect(label.contains("rad"),"radian unit shown");
        canvas.setAngleMode(AngleMode::Degrees);
        settle(canvas);
        hover(canvas,-2,0);
        expect(label.contains("deg"),"degree unit shown");
        canvas.setAngleMode(AngleMode::Radians);
    }
    {
        VariableScope both;
        expect(both.declare("y"),"implicit y declared");
        GraphCurve curve{QColor("#ffff00"),std::make_shared<Expression>("x^2+y^2-4",both),true,"circle implicit"};
        curve.implicitCurve=true;
        canvas.setCurves({curve});
        settle(canvas);
        expect(canvas.visibleGeometryReady(),"implicit ready");
        hover(canvas,2,0);
        expect(label.contains("Implicit"),"implicit identified");
        expect(label.contains("|F-G|"),"implicit residual reported");
    }
    {
        GraphCurve curve{QColor("#ff00ff"),std::make_shared<Expression>("x^2"),true,"y>x^2"};
        curve.inequalityCurve=true; curve.inequalityAbove=true; curve.inequalityInclusive=false;
        canvas.setCurves({curve});
        settle(canvas);
        hover(canvas,1,1);
        expect(label.contains("Inequality boundary"),"inequality boundary identified");
        expect(label.contains("strict"),"strict boundary disclosed");
        curve.inequalityInclusive=true;
        canvas.setCurves({curve});
        settle(canvas);
        hover(canvas,1,1);
        expect(label.contains("inclusive"),"inclusive boundary disclosed");
    }
    canvas.setCurves({});
    hover(canvas,1,1);
    expect(label.contains("x =") && !label.contains("boundary"),"fallback world coordinate");
    std::cout<<(failures?"FAIL: ":"PASS: ")<<checks<<" GraphTracing assertions\n";
    return failures?1:0;
}
