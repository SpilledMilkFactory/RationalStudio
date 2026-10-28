#include "PolarExpression.hpp"
#include "Expression.hpp"
#include "ParametricSampler.hpp"
#include "GraphViewport.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
using namespace rationalstudio;
namespace {
int assertions=0, failures=0;
void check(bool v, const char* s) {++assertions; if(!v) {++failures;std::cerr<<"FAIL: "<<s<<'\n';}}
constexpr double pi=3.1415926535897932384626433832795;
ParametricPoint evaluate(const Expression& x,const Expression& y,double t,AngleMode mode,VariableBindings bindings={}) {
    (void)bindings.set("t", t);
    return {x.evaluate(0,0,mode,bindings), y.evaluate(0,0,mode,bindings)};
}
}
int main() {
    const auto constant=polarCoordinateSources("r = 2");
    check(constant.valid() && constant.radius=="2", "constant radius");
    check(constant.x=="(2)*cos(t)" && constant.y=="(2)*sin(t)", "polar conversion formulas");
    check(polarCoordinateSources("r=2cos(θ)").radius=="2cos(t)", "Greek theta conversion");
    check(polarCoordinateSources("R=2cos(theta)").radius=="2cos(t)", "ASCII theta conversion");
    check(polarCoordinateSources("r=cos(THETA)").radius=="cos(t)", "case-insensitive angle alias");
    check(polarCoordinateSources("r=theta_squared").radius=="theta_squared", "only whole identifiers replaced");
    check(polarCoordinateSources("r=2π + θ²").radius=="2pi + t^2", "Unicode symbols normalized");
    check(!polarCoordinateSources("x=θ").valid(), "invalid polar label");
    check(!polarCoordinateSources("r=").valid(), "empty radius rejected");
    check(!polarCoordinateSources(" ").valid(), "empty text rejected");
    VariableScope local;
    check(local.declare("t"), "local angle parameter");
    const auto circle=polarCoordinateSources("r=2");
    Expression cx(circle.x,local),cy(circle.y,local);
    check(cx.valid() && cy.valid(), "circle coordinate expressions compile");
    for (int i=0;i<=64;++i) {
        const double t=2*pi*i/64;
        auto point=evaluate(cx,cy,t,AngleMode::Radians);
        check(std::abs(point.x-2*std::cos(t))<1e-11, "circle x independent oracle");
        check(std::abs(point.y-2*std::sin(t))<1e-11, "circle y independent oracle");
        check(std::abs(std::hypot(point.x,point.y)-2)<1e-10, "circle radius");
    }
    const auto shifted=polarCoordinateSources("r=2cos(θ)");
    Expression sx(shifted.x,local),sy(shifted.y,local);
    check(sx.valid() && sy.valid(), "offset circle expressions compile");
    for (int i=0;i<=96;++i) {
        const double t=2*pi*i/96;
        const auto p=evaluate(sx,sy,t,AngleMode::Radians);
        check(std::abs(std::hypot(p.x-1,p.y)-1)<1e-10, "r=2cos theta circle centered at (1,0)");
    }
    auto rose=polarCoordinateSources("r=2cos(3θ)");
    Expression rx(rose.x,local),ry(rose.y,local);
    check(rx.valid() && ry.valid(), "rose expressions compile");
    for(int i=0;i<=96;++i) {
        const double t=2*pi*i/96;
        const auto p=evaluate(rx,ry,t,AngleMode::Radians);
        const double r=2*std::cos(3*t);
        check(std::abs(p.x-r*std::cos(t))<1e-10, "negative radius x correct");
        check(std::abs(p.y-r*std::sin(t))<1e-10, "negative radius y correct");
    }
    const auto d=evaluate(rx,ry,60,AngleMode::Degrees);
    check(std::abs(d.x+1)<1e-10 && std::abs(d.y+std::sqrt(3.0))<1e-10,
          "degrees correctly convert radius and angular coordinates");
    const auto scalar=polarCoordinateSources("r=a*cos(θ)");
    VariableScope scope;
    check(scope.declare("t") && scope.declare("a"), "angle and slider scope");
    Expression scalarX(scalar.x,scope),scalarY(scalar.y,scope);
    check(scalarX.valid() && scalarY.valid(), "slider radius compiles");
    VariableBindings a;
    check(a.set("a",3), "binding set");
    const auto scaled=evaluate(scalarX,scalarY,pi/4,AngleMode::Radians,a);
    check(std::abs(scaled.x-1.5)<1e-10 && std::abs(scaled.y-1.5)<1e-10,
          "slider captured consistently");
    GraphViewport viewport;
    check(viewport.setDimensions(600,500) && viewport.setView(0,0,20), "sampling viewport");
    const auto samples=sampleParametricCurve(viewport,[&](double t) {
        return evaluate(sx,sy,t,AngleMode::Radians);
    },0,2*pi,65536);
    check(!samples.vertices.empty(), "polar sampler uses existing screen-space vertices");
    check(!samples.truncated, "reference polar circle completes within budget");
    CurveEvaluationBudget budget(96);
    const auto partial=sampleParametricCurve(viewport,[&](double t) {
        return evaluate(rx,ry,t,AngleMode::Radians);
    },0,2*pi,65536,&budget);
    check(budget.consumed()<=96, "polar respects shared evaluation budget");
    check(partial.evaluations<=96, "polar callback count bounded");
    check(!validParameterInterval(1,1) && !validParameterInterval(2,1), "invalid angle range rejected");
    std::cout<<(failures?"FAIL: ":"PASS: ")<<assertions<<" PolarExpression assertions\n";
    return failures?1:0;
}
