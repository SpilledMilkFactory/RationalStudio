// M3-05 — analytical numerical feature inspection and safety regressions.
#include "GraphFeatureDetection.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <string>

using namespace rationalstudio;
namespace {
int checks=0, failures=0;
void check(bool okay,const std::string& message) {
    ++checks;
    if(!okay){++failures;std::cerr<<"FAIL: "<<message<<'\n';}
}
const GraphFeaturePoint* findNear(const GraphFeatureResult& r,GraphFeatureKind kind,
                                  double x,double tol) {
    for(const auto& p:r.points)
        if(p.kind==kind&&std::abs(p.x-x)<tol) return &p;
    return nullptr;
}
bool near(const GraphFeatureResult& r,GraphFeatureKind kind,double x,double y,double tol) {
    const auto* p=findNear(r,kind,x,tol);
    return p&&std::abs(p->y-y)<tol;
}
}
int main() {
    const GraphFeatureFunction quad=[](double x){return x*x-1;};
    const auto r=inspectCartesianFeatures(quad,-3,3);
    check(r.complete(),"quadratic complete");
    check(near(r,GraphFeatureKind::Root,-1,0,1e-5),"negative quadratic root");
    check(near(r,GraphFeatureKind::Root,1,0,1e-5),"positive quadratic root");
    check(near(r,GraphFeatureKind::LocalMinimum,0,-1,1e-5),"quadratic minimum");
    check(!findNear(r,GraphFeatureKind::LocalMaximum,0,1),"no false quadratic maximum");
    check(r.evaluations<=GraphFeatureOptions{}.maxEvaluations,"overall evaluation accounting");
    for(const auto& p:r.points) {
        check(std::isfinite(p.x)&&std::isfinite(p.y),"all coordinates finite");
        check(p.xBracketWidth>=0,"bracket width nonnegative");
    }
    {
        const auto f=[](double x){return 4-x*x;};
        const auto q=inspectCartesianFeatures(f,-4,4);
        check(q.complete(),"inverted parabola complete");
        check(near(q,GraphFeatureKind::LocalMaximum,0,4,1e-5),"parabolic maximum");
        check(near(q,GraphFeatureKind::Root,-2,0,1e-5),"left maximum roots");
        check(near(q,GraphFeatureKind::Root,2,0,1e-5),"right maximum roots");
    }
    {
        const auto f=[](double x){return x*x*x;};
        const auto q=inspectCartesianFeatures(f,-2,2);
        check(q.complete(),"cubic complete");
        check(near(q,GraphFeatureKind::StationaryInflection,0,0,1e-4),"cubic stationary inflection");
        check(!findNear(q,GraphFeatureKind::LocalMinimum,0,.1),"cubic not minimum");
        check(!findNear(q,GraphFeatureKind::LocalMaximum,0,.1),"cubic not maximum");
    }
    {
        const auto f=[](double x){return std::pow(x-0.371,3)+2;};
        const auto q=inspectCartesianFeatures(f,-1,1);
        check(q.complete(),"shifted cubic complete");
        check(near(q,GraphFeatureKind::StationaryInflection,.371,2,1e-3),"unsampled stationary inflection");
    }
    {
        const auto f=[](double x){return std::abs(x-.371);};
        const auto q=inspectCartesianFeatures(f,-1,1);
        check(q.complete(),"cusp complete");
        check(near(q,GraphFeatureKind::LocalMinimum,.371,0,2e-4),"non-differentiable cusp minimum");
    }
    {
        const auto f=[](double x){return std::sin(x);};
        const auto q=inspectCartesianFeatures(f,-7,7);
        check(q.complete(),"sine complete");
        for(int k=-2;k<=2;++k)
            check(near(q,GraphFeatureKind::Root,k*std::numbers::pi,0,1e-5),"sine root");
        check(near(q,GraphFeatureKind::LocalMaximum,-1.5*std::numbers::pi,1,1e-4),"sine max left");
        check(near(q,GraphFeatureKind::LocalMaximum,.5*std::numbers::pi,1,1e-4),"sine max right");
        check(near(q,GraphFeatureKind::LocalMinimum,-.5*std::numbers::pi,-1,1e-4),"sine min left");
        check(near(q,GraphFeatureKind::LocalMinimum,1.5*std::numbers::pi,-1,1e-4),"sine min right");
    }
    {
        const auto f=[](double x){return x;};
        const auto q=inspectCartesianFeatures(f,-2,2);
        check(q.complete(),"line complete");
        check(q.points.size()==1&&q.points[0].kind==GraphFeatureKind::Root,"line root only");
    }
    {
        const auto q=inspectCartesianFeatures([](double){return 2.;},-1,1);
        check(q.complete() && q.points.empty(),"constant nonzero no isolated features");
        const auto all=inspectCartesianFeatures([](double){return 0.;},-1,1);
        check(all.status==GraphFeatureStatus::CoincidentWithAxis && all.points.empty(),
              "identically zero sampled domain is not list of isolated roots");
    }
    {
        const auto f=[](double x){return x==0?std::numeric_limits<double>::quiet_NaN():1/x;};
        const auto q=inspectCartesianFeatures(f,-1,1);
        check(q.complete(),"reciprocal finite scan");
        check(q.points.empty(),"no fake reciprocal pole roots or extrema");
    }
    {
        const auto q=inspectCartesianFeatures([](double x){return std::floor(x);},-2,2);
        check(q.status==GraphFeatureStatus::CoincidentWithAxis,
              "sampled flat zero interval explicitly reported");
        check(q.points.empty(),"flat zero interval not misreported as many isolated roots");
    }
    {
        const auto q=inspectCartesianFeatures([](double x){return std::tan(x);},-2,2);
        check(q.complete(),"tangent scan bounded and complete");
        check(near(q,GraphFeatureKind::Root,0,0,1e-6),"tangent true origin zero");
        check(!findNear(q,GraphFeatureKind::LocalMinimum,1.570796,0.02),
              "tangent pole not classified minimum");
        check(!findNear(q,GraphFeatureKind::LocalMaximum,-1.570796,0.02),
              "tangent pole not classified maximum");
    }
    {
        const auto q=inspectCartesianFeatures([](double x){return 1./(x-.135);},-2,2);
        check(q.complete(),"non-grid-aligned reciprocal scan complete");
        check(q.points.empty(),"non-grid-aligned pole not classified as extremum");
    }
    {
        const auto q=inspectCartesianFeatures([](double x){return x*x*x*x*x;},-1,1);
        check(near(q,GraphFeatureKind::StationaryInflection,0,0,1e-4),
              "quintic flat inflection");
        const auto p=inspectCartesianFeatures([](double x){return x*x*x*x;},-1,1);
        check(near(p,GraphFeatureKind::LocalMinimum,0,0,1e-4),
              "quartic flat local minimum");
    }
    {
        const auto f=[](double x){return x<0?-1.:1.;};
        const auto q=inspectCartesianFeatures(f,-1,1);
        check(q.complete(),"step finite scan");
        check(q.points.empty(),"no phantom discontinuity roots");
    }
    {
        const auto f=[](double x){return x*x;};
        auto o=GraphFeatureOptions{};
        o.maxEvaluations=32;
        const auto q=inspectCartesianFeatures(f,-1,1,o);
        check(q.status==GraphFeatureStatus::EvaluationLimit,"evaluation limit explicit");
        check(q.evaluations<=32,"evaluation limit respected");
        o=GraphFeatureOptions{};o.maxResults=1;
        const auto capped=inspectCartesianFeatures(quad,-2,2,o);
        check(capped.status==GraphFeatureStatus::ResultLimit,"result cap explicit");
        check(capped.points.size()<=1,"result cap respected");
        o=GraphFeatureOptions{};o.subdivisions=1;
        check(inspectCartesianFeatures(f,-1,1,o).status==GraphFeatureStatus::InvalidInput,"reject too few samples");
        o=GraphFeatureOptions{};o.derivativeTolerance=0;
        check(inspectCartesianFeatures(f,-1,1,o).status==GraphFeatureStatus::InvalidInput,"reject zero derivative tolerance");
        check(inspectCartesianFeatures(f,1,1).status==GraphFeatureStatus::InvalidInput,"reject zero width");
        check(inspectCartesianFeatures(f,1,-1).status==GraphFeatureStatus::InvalidInput,"reject reversed domain");
        check(inspectCartesianFeatures(f,-1,std::numeric_limits<double>::infinity()).status==GraphFeatureStatus::InvalidInput,"reject unbounded domain");
        check(inspectCartesianFeatures(GraphFeatureFunction{},-1,1).status==GraphFeatureStatus::InvalidInput,"reject empty callback");
    }
    {
        Expression f("x^2-a");
        VariableScope scope;scope.declare("a");
        Expression dynamic("x^2-a",scope);
        check(!f.valid()&&dynamic.valid(),"declared variable scope honored");
        VariableBindings first;first.set("a",1);
        VariableBindings second;second.set("a",4);
        const auto x=inspectCartesianFeatures(dynamic,-3,3,AngleMode::Radians,first);
        const auto y=inspectCartesianFeatures(dynamic,-3,3,AngleMode::Radians,second);
        check(near(x,GraphFeatureKind::Root,1,0,1e-5),"bindings snapshot A");
        check(near(y,GraphFeatureKind::Root,2,0,1e-5),"bindings snapshot B");
        check(inspectCartesianFeatures(f,-3,3,AngleMode::Radians,first).status==GraphFeatureStatus::InvalidInput,
              "reject invalid compiled expression");
    }
    std::cout<<(failures?"FAIL: ":"PASS: ")<<checks<<" GraphFeatureDetection assertions\n";
    return failures?1:0;
}
