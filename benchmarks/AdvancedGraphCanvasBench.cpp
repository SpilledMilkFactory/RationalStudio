// Issue #45: optional Qt offscreen graph latency and warm-cache observations.
// Run with QT_QPA_PLATFORM=offscreen; results are NOT physical display latency.
#include "GraphCanvas.hpp"
#include "ImplicitContour.hpp"
#include "PolarExpression.hpp"
#include "Expression.hpp"
#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QTimer>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
using namespace rationalstudio;
namespace {
using clock_type=std::chrono::steady_clock;
constexpr double pi=3.1415926535897932384626433832795;
std::shared_ptr<const Expression> regular(const char* s) { auto e=std::make_shared<Expression>(s); return e->valid()?e:nullptr; }
GraphCurve single(const char* name,const char* formula) {
    return GraphCurve{QColor("#66ccee"),regular(formula),true,QString::fromUtf8(name)};
}
GraphCurve parametric(bool polar) {
    VariableScope scope;
    (void)scope.declare("t");
    std::string x="3*cos(t)",y="3*sin(t)";
    if(polar) {
        const auto src=polarCoordinateSources("r=2cos(3theta)");
        x=src.x;y=src.y;
    }
    auto xx=std::make_shared<Expression>(x,scope),yy=std::make_shared<Expression>(y,scope);
    GraphCurve curve{QColor("#ee8866"),xx,true,"sample",yy,0,2*pi};
    curve.polarCurve=polar;
    return curve;
}
GraphCurve implicit() {
    VariableScope scope;
    (void)scope.declare("y");
    const auto src=implicitEquationSource("x^2+y^2=9");
    GraphCurve curve{QColor("#cce077"),std::make_shared<Expression>(src.expression,scope),true,"implicit"};
    curve.implicitCurve=true;
    return curve;
}
GraphCurve inequality() {
    GraphCurve curve=single("inequality","x^2-1");
    curve.inequalityCurve=true;
    curve.inequalityAbove=true;
    return curve;
}
void pump() {
    QEventLoop events;
    QTimer::singleShot(8,&events,&QEventLoop::quit);
    events.exec();
}
bool ready(GraphCanvas& canvas,int timeoutMs=12000) {
    QElapsedTimer timer;timer.start();
    while(timer.elapsed()<timeoutMs) {
        (void)canvas.grab();
        QApplication::processEvents();
        if(canvas.visibleGeometryReady()&&canvas.visibleInequalityReady())return true;
        pump();
    }
    return false;
}
void print(const std::string& name,std::vector<double> times,
           std::size_t builds,std::size_t callbacks) {
    std::sort(times.begin(),times.end());
    const auto rank=static_cast<std::size_t>(std::ceil(0.95*times.size()))-1;
    std::cout<<name<<','<<times.size()<<','<<std::fixed<<std::setprecision(6)
             <<times[times.size()/2]<<','<<times[rank]<<','<<times.back()
             <<','<<callbacks<<','<<builds<<'\n';
}
}
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    int repetitions=7;
    if(argc==2) {
        try {repetitions=std::stoi(argv[1]);}catch(...){return 2;}
        if(repetitions<5||repetitions>60)return 2;
    } else if(argc!=1)return 2;
    std::cout<<"case,repetitions,median_ms,p95_ms,max_ms,observed_callback_delta,observed_geometry_build_delta\n";
    for(int kind=0;kind<5;++kind) {
        const std::string label=std::vector<std::string>{"cartesian","parametric","polar","implicit","inequality"}[kind];
        GraphCurve curve;
        if(kind==0)curve=single("cartesian","sin(x)+0.1*cos(8*x)");
        if(kind==1)curve=parametric(false);
        if(kind==2)curve=parametric(true);
        if(kind==3)curve=implicit();
        if(kind==4)curve=inequality();
        if(!curve.function||!curve.function->valid()||(curve.yCoordinate&&!curve.yCoordinate->valid())) {
            std::cerr<<"Invalid benchmark graph "<<label<<'\n';return 1;
        }
        GraphCanvas canvas;
        canvas.resize(720,480);
        canvas.show();
        QApplication::processEvents();
        canvas.setCurves({curve});
        if(!ready(canvas)) {std::cerr<<"Initial graph not ready: "<<label<<'\n';return 1;}
        // Time a changed-view rebuild to completion, not only the first paint.
        std::vector<double> cold,warm;
        std::size_t changes=0,callbacks=0;
        for(int i=0;i<repetitions;++i) {
            const double centerX=0.0125*(i+1);
            const auto beforeBuilds=canvas.geometryBuildCount();
            const auto beforeCallbacks=canvas.samplingDiagnostics().totalEvaluations;
            const auto beforeShade=canvas.inequalityEvaluationCount();
            const auto t0=clock_type::now();
            canvas.setView(centerX,0,55);
            if(!ready(canvas)) {std::cerr<<"Cold rebuild timeout: "<<label<<'\n';return 1;}
            const auto t1=clock_type::now();
            cold.push_back(std::chrono::duration<double,std::milli>(t1-t0).count());
            changes+=canvas.geometryBuildCount()-beforeBuilds;
            callbacks+=(canvas.samplingDiagnostics().totalEvaluations-beforeCallbacks)
                      +(canvas.inequalityEvaluationCount()-beforeShade);
            const auto warmBuilds=canvas.geometryBuildCount();
            const auto warmSamples=canvas.samplingDiagnostics().totalEvaluations;
            const auto warmShade=canvas.inequalityEvaluationCount();
            const auto t2=clock_type::now();
            (void)canvas.grab();
            const auto t3=clock_type::now();
            warm.push_back(std::chrono::duration<double,std::milli>(t3-t2).count());
            if(canvas.geometryBuildCount()!=warmBuilds ||
               canvas.samplingDiagnostics().totalEvaluations!=warmSamples ||
               canvas.inequalityEvaluationCount()!=warmShade) {
                std::cerr<<"Warm raster invalidated completed geometry: "<<label<<'\n';return 1;
            }
        }
        print("qt_"+label+"_cold_to_ready",cold,changes,callbacks);
        print("qt_"+label+"_warm_repaint",warm,0,0);
    }
    return 0;
}
