// Issue #45: independent mathematical oracles across all supported graph types.
// Tests numeric approximation, not symbolic completeness or raster pixel identity.
#include "CurveSampler.hpp"
#include "ParametricSampler.hpp"
#include "PolarExpression.hpp"
#include "ImplicitContour.hpp"
#include "InequalityRegion.hpp"
#include "GraphTrace.hpp"
#include "IntersectionDetection.hpp"
#include "GraphFeatureDetection.hpp"
#include "Expression.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <string>

using namespace rationalstudio;
namespace {
int assertions=0, failures=0;
constexpr double pi=3.1415926535897932384626433832795;
void check(bool success, const std::string& description) {
    ++assertions;
    if (!success) { ++failures; std::cerr << "FAIL: " << description << '\n'; }
}
bool close(double a,double b,double tolerance) { return std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<=tolerance; }
GraphViewport viewport(int width=640,int height=480,double pixels=45.0) {
    GraphViewport v;
    if (!v.setDimensions(width,height) || !v.setView(0,0,pixels)) std::abort();
    return v;
}
void cartesian() {
    auto view=viewport();
    const auto linear=sampleExplicitCurve(view, [](double x){return 0.75*x-0.125;});
    check(!linear.truncated && linear.vertices.size()>2,"Cartesian linear sampling completed");
    for (const auto& v:linear.vertices) {
        const double x=view.screenToWorldX(v.screenX);
        const double y=view.screenToWorldY(v.screenY);
        check(close(y,0.75*x-0.125,1e-8),"Cartesian linear vertex matches independent oracle");
    }
    const auto parabola=sampleExplicitCurve(view,[](double x){return x*x-1;});
    check(!parabola.truncated && !parabola.vertices.empty(),"Cartesian parabola sampled");
    for (const auto& v:parabola.vertices) {
        const double x=view.screenToWorldX(v.screenX),y=view.screenToWorldY(v.screenY);
        check(close(y,x*x-1,2e-7),"Cartesian parabola vertex matches oracle");
    }
}
void parametric() {
    auto view=viewport();
    const auto circle=sampleParametricCurve(view,[](double t){return ParametricPoint{3*std::cos(t),3*std::sin(t)};},0,2*pi);
    check(!circle.truncated && circle.vertices.size()>80,"parametric circle sampled completely");
    for (const auto& v:circle.vertices) {
        const double x=view.screenToWorldX(v.screenX),y=view.screenToWorldY(v.screenY);
        check(close(std::hypot(x,y),3,2e-7),"parametric circle radius oracle");
        check(close(x,3*std::cos(v.parameter),2e-7) && close(y,3*std::sin(v.parameter),2e-7),
              "parametric vertex retains its generating t");
    }
    std::mt19937_64 generator(0x4545abcdULL);
    std::uniform_real_distribution<double> angles(0,2*pi);
    for (int i=0;i<96;++i) {
        const double t=angles(generator);
        check(close(std::hypot(3*std::cos(t),3*std::sin(t)),3,1e-12),"seeded circle reference oracle");
    }
}
void polar() {
    const auto source=polarCoordinateSources("r=2cos(3theta)");
    check(source.valid(),"polar rose input supported");
    VariableScope scope;
    check(scope.declare("t"),"polar local parameter declared");
    Expression x(source.x,scope),y(source.y,scope);
    check(x.valid()&&y.valid(),"polar adapter expressions compile");
    auto view=viewport(700,500,42);
    const auto sampled=sampleParametricCurve(view,[&](double t) {
        VariableBindings bindings;
        (void)bindings.set("t",t);
        return ParametricPoint{x.evaluate(0,0,AngleMode::Radians,bindings),
                               y.evaluate(0,0,AngleMode::Radians,bindings)};
    },0,2*pi);
    check(!sampled.truncated&&!sampled.vertices.empty(),"polar rose fully sampled");
    bool sawNegative=false;
    for (const auto& v:sampled.vertices) {
        const double r=2*std::cos(3*v.parameter);
        if (r<-0.2) sawNegative=true;
        check(close(view.screenToWorldX(v.screenX),r*std::cos(v.parameter),2e-7) &&
              close(view.screenToWorldY(v.screenY),r*std::sin(v.parameter),2e-7),
              "polar signed-radius Cartesian oracle");
    }
    check(sawNegative,"polar negative-radius branch exercised");
}
void implicit() {
    auto view=viewport(480,480,32);
    const auto circle=sampleImplicitContour(view,[](double x,double y){return x*x+y*y-9;},8,100000);
    check(!circle.truncated&&circle.vertices.size()>60,"implicit circle completed");
    for (const auto& v:circle.vertices) {
        const double x=view.screenToWorldX(v.screenX),y=view.screenToWorldY(v.screenY);
        check(std::abs(x*x+y*y-9)<0.5,"implicit circle residual within explicit grid tolerance");
    }
    const auto empty=sampleImplicitContour(view,[](double,double){return 1.;},8,100000);
    check(empty.vertices.empty()&&!empty.truncated,"implicit no-root field produces no invented contour");
}
void inequality() {
    for (const auto& source : {"y>x^2","x^2<y","y>=x^2","x^2<=y"}) {
        const auto parsed=parseInequality(source);
        check(parsed.valid() && parsed.shadeAbove,"inequality above direction and reverse syntax");
        const auto span=inequalityVerticalSpan(1.,-4.,4.,160.,parsed.shadeAbove);
        check(span && close(span->top,0,1e-12) && close(span->bottom,60,1e-9),
              "inequality above clipped upper span oracle");
    }
    for (const auto& source : {"y<x^2","x^2>y","y<=x^2","x^2>=y"}) {
        const auto parsed=parseInequality(source);
        check(parsed.valid() && !parsed.shadeAbove,"inequality below direction and reverse syntax");
        const auto span=inequalityVerticalSpan(1.,-4.,4.,160.,parsed.shadeAbove);
        check(span && close(span->top,60,1e-9)&&close(span->bottom,160,1e-12),
              "inequality below clipped lower span oracle");
    }
}
void analysisAndTrace() {
    const auto intersections=findCartesianIntersections([](double x){return x*x;},
                                                       [](double){return 1.;},-2,2);
    check(intersections.complete(),"Cartesian intersections complete bounded search");
    bool left=false,right=false;
    for (const auto& p:intersections.points) {
        check(std::abs(p.x*p.x-1)<1e-6 && p.residual<1e-6,"intersection oracle residual");
        left |= std::abs(p.x+1)<1e-5;
        right |= std::abs(p.x-1)<1e-5;
    }
    check(left&&right,"both analytical intersections found");
    const auto features=inspectCartesianFeatures([](double x){return x*x-1.;},-2,2);
    check(features.complete(),"feature inspection completes");
    bool minimum=false;
    for (const auto& p:features.points) {
        if (p.kind==GraphFeatureKind::LocalMinimum && std::abs(p.x)<0.01 && std::abs(p.y+1)<0.01) minimum=true;
    }
    check(minimum,"parabola minimum oracle");
    const std::vector<CurveVertex> segments{{0,0,true,0},{100,0,false,2},{200,100,true,4},{300,100,false,6}};
    const auto hit=nearestGraphTrace(segments,25,3);
    check(hit && hit->hasParameter&&close(hit->parameter,0.5,1e-10),"trace parameter interpolation oracle");
    check(!nearestGraphTrace(segments,150,50,20),"trace does not join disconnected branches");
}
}
int main() {
    cartesian();parametric();polar();implicit();inequality();analysisAndTrace();
    std::cout<<(failures?"FAIL: ":"PASS: ")<<assertions<<" AdvancedGraphAccuracyMatrix assertions\n";
    return failures?1:0;
}
