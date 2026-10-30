// Issue #45: deterministic evaluation budgets, resumption and numerical boundaries.
#include "CurveSampler.hpp"
#include "ParametricSampler.hpp"
#include "ImplicitContour.hpp"
#include "InequalityRegion.hpp"
#include "IntersectionDetection.hpp"
#include "GraphTrace.hpp"
#include <cmath>
#include <iostream>
#include <limits>
using namespace rationalstudio;
namespace {
int checks=0,failures=0;
void check(bool yes,const char* why){++checks;if(!yes){++failures;std::cerr<<"FAIL: "<<why<<'\n';}}
}
int main(){
    GraphViewport view;
    check(view.setDimensions(640,480)&&view.setView(0,0,32),"viewport configured");
    // Limits are actual evaluation accounting, not timing promises.
    CurveSamplingOptions options;
    options.maxEvaluations=48;
    const auto cart=sampleExplicitCurve(view,[](double x){return std::sin(30*x);},options);
    check(cart.evaluations<=48,"Cartesian per-call callback cap");
    check(cart.truncated,"Cartesian high-frequency budget exhaustion disclosed");
    CurveEvaluationBudget pbudget(48);
    const auto param=sampleParametricCurve(view,[](double t){return ParametricPoint{std::cos(23*t),std::sin(23*t)};},0,6.283185307179586,100000,&pbudget);
    check(pbudget.consumed()<=48&&param.evaluations<=48,"parametric/polar shared budget cap");
    check(param.truncated,"parametric/polar incomplete sampling disclosed");
    CurveEvaluationBudget ibudget(48);
    const auto implicit=sampleImplicitContour(view,[](double x,double y){return x*x+y*y-4;},8,100000,&ibudget);
    check(ibudget.consumed()<=48&&implicit.evaluations<=48,"implicit shared budget cap");
    check(implicit.truncated,"implicit unfinished contour disclosed");
    IntersectionOptions opts;
    opts.subdivisions=1024;
    opts.maxEvaluations=48;
    const auto result=findCartesianIntersections([](double x){return std::sin(x);},[](double){return 0.;},-10,10,opts);
    check(result.evaluations<=48,"intersection callback budget counts both functions");
    check(result.status==IntersectionStatus::EvaluationLimit,"intersection exhaustion status");
    const auto nan=std::numeric_limits<double>::quiet_NaN();
    const auto inf=std::numeric_limits<double>::infinity();
    for (const auto v:{nan,inf,-inf}) {
        check(!inequalityVerticalSpan(v,-5,5,100,true),"inequality nonfinite boundary rejected");
        check(!nearestGraphTrace({{0,0,true},{10,10,false}},v,0),"trace nonfinite cursor rejected");
    }
    // Numerical boundaries are checked without inventing geometry in undefined domains.
    const auto restricted=sampleExplicitCurve(view,[](double x) {
        return x<0 ? std::numeric_limits<double>::quiet_NaN() : std::sqrt(x);
    });
    bool noNegative=true;
    for(const auto& v:restricted.vertices)
        noNegative &= std::isfinite(v.screenX)&&std::isfinite(v.screenY) &&
                      view.screenToWorldX(v.screenX)>=-1e-8;
    check(!restricted.vertices.empty()&&noNegative,"restricted Cartesian domain never draws negative x");
    check(!validParameterInterval(0,inf),"nonfinite parameter interval rejected");
    const auto extreme=inequalityVerticalSpan(0,-1e300,1e300,200,true);
    check(extreme && std::abs(extreme->bottom-100)<1e-8,
          "extreme finite inequality viewport span remains stable");
    const auto brokenContour=sampleImplicitContour(view,[](double x,double y) {
        return x>0 && y>0 ? std::numeric_limits<double>::quiet_NaN() : x+y;
    },12,100000);
    bool finiteContour=true;
    for(const auto& v:brokenContour.vertices)
        finiteContour &= std::isfinite(v.screenX) && std::isfinite(v.screenY);
    check(!brokenContour.truncated && finiteContour,
          "implicit undefined quadrant never emits nonfinite geometry");
    const auto broken=sampleParametricCurve(view,[](double t){
        if(t>0.45&&t<0.55)return ParametricPoint{NAN,NAN};
        return ParametricPoint{t,t};
    },0,1);
    bool separated=false;
    for (const auto& v:broken.vertices) separated |= v.moveTo&&v.parameter>0.55;
    check(separated,"parameter-domain hole creates independent stroke");
    // Completed geometry must never be confused with the budget-truncated view.
    const auto complete=sampleParametricCurve(view,[](double t){return ParametricPoint{std::cos(t),std::sin(t)};},0,6.283185307179586);
    check(!complete.truncated&&complete.vertices.size()>param.vertices.size(),"full sampling distinct from limited sample");
    std::cout<<(failures?"FAIL: ":"PASS: ")<<checks<<" AdvancedGraphResourceMatrix assertions\n";
    return failures?1:0;
}
