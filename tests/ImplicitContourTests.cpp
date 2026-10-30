#include "ImplicitContour.hpp"
#include "Expression.hpp"
#include "MathExpressionInput.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

using namespace rationalstudio;
namespace {
int checks=0, failures=0;
void check(bool ok, const char* label) {
    ++checks;
    if(!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
constexpr double eps = 0.36;
}
int main() {
    auto parsed = implicitEquationSource(" x² + y² = 25 ");
    check(parsed.valid(), "Unicode circle equation accepted");
    check(parsed.expression.find("-(25)")!=std::string::npos,"subtract right-hand side");
    for (const auto& invalid : {"", "x^2+y^2", "x=", "=3", "x=y=z", "    =   "})
        check(!implicitEquationSource(invalid).valid(), "invalid equation label rejected");
    VariableScope scope;
    check(scope.declare("y"), "y declared in per-curve scope");
    Expression circle(MathExpressionInput::normalize(parsed.expression), scope);
    check(circle.valid(),"circle equation compiles against local y");
    check(circle.dependsOnX(),"implicit circle depends on x");
    VariableBindings base;
    auto evaluation = [&circle,base](double x,double y) mutable {
        auto bindings=base;
        (void)bindings.set("y",y);
        return circle.evaluate(x,0,AngleMode::Radians,bindings);
    };
    check(std::abs(evaluation(3,4))<1e-12,"circle zero at (3,4)");
    check(evaluation(0,0)<0,"circle interior negative");
    check(evaluation(6,0)>0,"circle exterior positive");
    GraphViewport view;
    check(view.setDimensions(480,480),"viewport dimensions set");
    check(view.setView(0,0,30),"viewport centered");
    const auto result=sampleImplicitContour(view,evaluation,9.0,500000);
    check(!result.truncated,"circle fully sampled");
    check(result.evaluations>0 && result.evaluations%4==0,"four corner probes per cell");
    check(result.vertices.size()>80,"circle contour has many segments");
    check(result.vertices.size()%2==0,"isolated segment pairs");
    int nearZeros=0;
    for (std::size_t i=0;i+1<result.vertices.size();i+=2) {
        check(result.vertices[i].moveTo && !result.vertices[i+1].moveTo,
              "segments preserve move-to/line-to contract");
        for (std::size_t k=i;k<i+2;++k) {
            const auto& v=result.vertices[k];
            const double x=view.screenToWorldX(v.screenX), y=view.screenToWorldY(v.screenY);
            check(std::isfinite(x)&&std::isfinite(y),"finite contour coordinates");
            if (std::abs(x*x+y*y-25)<eps) ++nearZeros;
        }
    }
    check(nearZeros > static_cast<int>(result.vertices.size()*0.90),
          "circle sampled vertices match analytical radius within grid tolerance");
    ImplicitContourJob incremental(view,evaluation,9.0);
    check(incremental.valid(),"incremental job valid");
    int rounds=0;
    std::size_t total=0;
    for (;;) {
        CurveEvaluationBudget budget(512);
        const bool done=incremental.advance(512,budget);
        check(budget.consumed()<=512,"per-paint shared budget held");
        total+=budget.consumed();
        if(done) break;
        if(++rounds>500) {check(false,"job completes within bounded rounds");break;}
    }
    check(rounds>0,"job yielded for resource budget");
    check(total==incremental.samples().evaluations,"callback accounting exact");
    check(incremental.samples().vertices.size()==result.vertices.size(),
          "resumed job equals single call geometry");
    if(incremental.samples().vertices.size()==result.vertices.size())
        for (std::size_t i=0;i<result.vertices.size();++i) {
            const auto& a=result.vertices[i],&b=incremental.samples().vertices[i];
            check(a.screenX==b.screenX&&a.screenY==b.screenY&&a.moveTo==b.moveTo,
                  "incremental and single-shot deterministic");
        }
    CurveEvaluationBudget limited(20);
    const auto truncated=sampleImplicitContour(view,evaluation,9.0,200,&limited);
    check(truncated.truncated,"shared budget reports truncated result");
    check(truncated.evaluations==20&&limited.consumed()==20,"shared limit exact");
    check(truncated.vertices.size()<result.vertices.size(),"limited sample is partial");
    const auto invalidStep=sampleImplicitContour(view,evaluation,0,100);
    check(invalidStep.evaluations==0 && invalidStep.vertices.empty(),"invalid step rejected");
    int calls=0;
    const auto broken=sampleImplicitContour(view,[&](double x,double y) {
        ++calls;
        return x>0&&y>0 ? std::numeric_limits<double>::quiet_NaN() : x+y;
    },12,250000);
    check(!broken.truncated,"nonfinite cells do not prevent completion");
    check(calls==static_cast<int>(broken.evaluations),"nonfinite callback counts stable");
    for(const auto& vertex:broken.vertices)
        check(std::isfinite(vertex.screenX)&&std::isfinite(vertex.screenY),
              "nonfinite field never produces nonfinite geometry");
    const auto saddle=sampleImplicitContour(view,[](double x,double y){return x*y-1;},8,250000);
    check(!saddle.truncated && !saddle.vertices.empty(),"disconnected hyperbola contour sampled");
    const auto empty=sampleImplicitContour(view,[](double,double){return 42.0;},12,250000);
    check(!empty.truncated && empty.vertices.empty(),"constant nonzero field empty contour");
    std::cout << (failures?"FAIL: ":"PASS: ")<<checks<<" ImplicitContour assertions\n";
    return failures?1:0;
}
