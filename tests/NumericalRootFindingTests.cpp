#include "NumericalRootFinding.hpp"
#include "FunctionDefinitions.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <random>
#include <stdexcept>
#include <string>

using namespace rationalstudio;
namespace {
int checks = 0;
int failures = 0;
void check(bool ok, const std::string& name) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}
void oracle(const char* name, const RootFunction& fn, double a, double b,
            double expected, double tolerance = 1e-7) {
    const auto r = solveBracketedRoot(fn, a, b);
    if (!r.converged())
        std::cerr << "INFO: " << name << ": " << rootStatusName(r.status)
                  << " evaluations=" << r.evaluations
                  << " iterations=" << r.iterations << '\n';
    check(r.converged(), std::string(name) + " converged");
    check(r.converged() && std::abs(r.x - expected) <= tolerance,
          std::string(name) + " coordinate oracle");
    check(r.converged() && std::isfinite(r.residual) &&
          r.residual <= RootOptions{}.residualTolerance,
          std::string(name) + " residual acceptance");
    check(r.evaluations <= RootOptions{}.maxEvaluations,
          std::string(name) + " evaluation cap");
    check(r.iterations <= RootOptions{}.maxIterations,
          std::string(name) + " iteration cap");
}
}
int main() {
    constexpr double pi = std::numbers::pi;
    oracle("linear", [](double x){ return 3*x-6; }, -1, 5, 2);
    oracle("quadratic positive", [](double x){ return x*x-2; }, 0, 4, std::sqrt(2.));
    oracle("quadratic negative", [](double x){ return x*x-2; }, -4, 0, -std::sqrt(2.));
    oracle("cubic", [](double x){ return x*x*x-8; }, 1, 4, 2);
    oracle("sine pi", [](double x){ return std::sin(x); }, 2, 4, pi);
    oracle("cosine half pi", [](double x){ return std::cos(x); }, 1, 2, pi/2);
    oracle("exponential", [](double x){ return std::exp(x)-3; }, 0, 3, std::log(3.));
    oracle("logarithm", [](double x){ return std::log(x)-1; }, 1, 4, std::numbers::e);
    oracle("atan", [](double x){ return std::atan(x)-.5; }, 0, 2, std::tan(.5));
    oracle("large-offset", [](double x){return x-1e12-2.;}, 1e12, 1e12+6, 1e12+2, 1e-3);
    {
        auto r = solveBracketedRoot([](double x){return x-2;},2,6);
        check(r.converged() && r.x==2, "left endpoint exactly zero");
        check(r.evaluations==2 && r.iterations==0, "left endpoint callback accounting");
        auto s = solveBracketedRoot([](double x){return x-6;},2,6);
        check(s.converged() && s.x==6,"right endpoint exactly zero");
    }
    {
        auto r=solveBracketedRoot([](double x){return x*x+1;},-2,2);
        check(r.status==RootStatus::NoSignChange,"no real root/no sign change");
        check(!std::isfinite(r.x),"failed root not published");
        auto tangent=solveBracketedRoot([](double x){return x*x;},-1,1);
        check(tangent.status==RootStatus::NoSignChange,
              "off-endpoint double root not claimed bracketed");
    }
    {
        auto undef=solveBracketedRoot([](double x){return std::log(x);},-1,3);
        check(undef.status==RootStatus::UndefinedEndpoint,"invalid endpoint rejected");
        check(undef.evaluations==2,"invalid endpoints count both callbacks");
        auto throwing=solveBracketedRoot([](double){throw std::runtime_error("error"); return 0.;},-1,1);
        check(throwing.status==RootStatus::UndefinedEndpoint,"callback exception rejected");
        auto pole=solveBracketedRoot([](double x){return 1/(x-.3);},0,1);
        check(!pole.converged(),"discontinuous sign change not accepted as root");
        check(!std::isfinite(pole.x),"discontinuity not published as numerical solution");
        auto singular=solveBracketedRoot([](double x){return 1/x;},-1,1);
        check(!singular.converged(),"pole at midpoint not accepted");
    }
    {
        RootOptions opt; opt.maxEvaluations=2;
        const auto r=solveBracketedRoot([](double x){return x*x-2;},0,3,opt);
        check(r.status==RootStatus::EvaluationLimit,"strict evaluation budget");
        check(r.evaluations==2,"no extra callbacks past budget");
        opt=RootOptions{};opt.maxIterations=1;
        const auto limited=solveBracketedRoot([](double x){return std::sin(x);},2,4,opt);
        check(limited.status==RootStatus::IterationLimit,"iteration cap status");
        check(limited.iterations==1,"iteration cap exact");
        opt=RootOptions{};opt.xAbsoluteTolerance=5.;
        auto unresolved=solveBracketedRoot([](double x){return x*x-2;},0,3,opt);
        check(unresolved.status==RootStatus::Unresolved,"tight bracket alone not sufficient");
        check(!std::isfinite(unresolved.x),"unresolved coordinate not published");
    }
    {
        const auto inf=std::numeric_limits<double>::infinity();
        check(solveBracketedRoot(RootFunction{},-1,1).status==RootStatus::InvalidInput,
              "empty function input");
        check(solveBracketedRoot([](double x){return x;},1,1).status==RootStatus::InvalidInput,
              "equal bounds invalid");
        check(solveBracketedRoot([](double x){return x;},2,-1).status==RootStatus::InvalidInput,
              "reversed bracket invalid");
        check(solveBracketedRoot([](double x){return x;},0,inf).status==RootStatus::InvalidInput,
              "nonfinite endpoint invalid");
        check(solveBracketedRoot([](double x){return x;},-1e308,1e308).status==RootStatus::InvalidInput,
              "overflowing span invalid");
        RootOptions o;o.residualTolerance=-1;
        check(solveBracketedRoot([](double x){return x;},-1,1,o).status==RootStatus::InvalidInput,
              "negative residual tolerance invalid");
        o=RootOptions{};o.xAbsoluteTolerance=0;
        check(solveBracketedRoot([](double x){return x;},-1,1,o).status==RootStatus::InvalidInput,
              "zero x tolerance invalid");
        o=RootOptions{};o.maxIterations=0;
        check(solveBracketedRoot([](double x){return x;},-1,1,o).status==RootStatus::InvalidInput,
              "zero iteration cap invalid");
        o=RootOptions{};o.maxEvaluations=1;
        check(solveBracketedRoot([](double x){return x;},-1,1,o).status==RootStatus::InvalidInput,
              "fewer than two samples invalid");
    }
    {
        auto roots=findRootsInDomain([](double x){return x*x-1;},-3,3);
        check(roots.complete(),"quadratic domain search completed");
        check(roots.roots.size()==2,"two quadratic roots found");
        if (roots.roots.size()==2) {
            check(std::abs(roots.roots[0].x+1)<1e-7,"domain root -1");
            check(std::abs(roots.roots[1].x-1)<1e-7,"domain root +1");
        }
        check(roots.evaluations<=RootSearchOptions{}.maxEvaluations,
              "search global callback budget respected");
        auto sine=findRootsInDomain([](double x){return std::sin(x);},-4,4);
        check(sine.complete() && sine.roots.size()==3,"sine sampled roots -pi, 0, pi");
        auto tangent=findRootsInDomain([](double x){return (x-.7)*(x-.7);},0,2);
        check(tangent.roots.empty(),"off-grid even root not invented");
        auto domain=findRootsInDomain([](double x){return std::sqrt(x)-1;},-1,4);
        check(domain.status==RootSearchStatus::IncompleteDomain,"undefined sampled domain is incomplete");
        check(domain.invalidSamples>0,"undefined samples counted");
        check(!domain.roots.empty(),"valid root still discovered in partial domain");
    }
    {
        RootSearchOptions o;o.maxEvaluations=5;
        auto r=findRootsInDomain([](double x){return x-3;},0,5,o);
        check(r.status==RootSearchStatus::EvaluationLimit,"global search budget status");
        check(r.evaluations==5,"global search never exceeds callback budget");
        o=RootSearchOptions{};o.maxResults=1;
        auto limited=findRootsInDomain([](double x){return x*x-1;},-2,2,o);
        check(limited.status==RootSearchStatus::ResultLimit,"search result cap status");
        check(limited.roots.size()==1,"search result ceiling obeyed");
        o=RootSearchOptions{};o.subdivisions=1;
        check(findRootsInDomain([](double x){return x;},-1,1,o).status==RootSearchStatus::InvalidInput,
              "too few scan subdivisions invalid");
        o=RootSearchOptions{};o.maxResults=0;
        check(findRootsInDomain([](double x){return x;},-1,1,o).status==RootSearchStatus::InvalidInput,
              "zero result cap invalid");
    }
    {
        VariableBindings empty;
        Expression f("x^2-2");
        auto root=solveBracketedRoot(f,0,2,AngleMode::Radians,empty);
        check(root.converged()&&std::abs(root.x-std::sqrt(2.))<1e-7,
              "compiled expression root");
        Expression lhs("x^2"),rhs("2");
        auto equation=solveEquation(lhs,rhs,0,2,AngleMode::Radians,empty);
        check(equation.converged()&&std::abs(equation.x-std::sqrt(2.))<1e-7,
              "equation left equals right");
        Expression invalid("unknown(x)");
        check(solveBracketedRoot(invalid,0,2,AngleMode::Radians,empty).status==RootStatus::InvalidInput,
              "invalid expression rejected");
        check(solveEquation(lhs,invalid,0,2,AngleMode::Radians,empty).status==RootStatus::InvalidInput,
              "invalid equation side rejected");
        auto deg=solveBracketedRoot(Expression("sin(x)-0.5"),10,50,AngleMode::Degrees,empty);
        check(deg.converged()&&std::abs(deg.x-30)<1e-7,"degree-mode trigonometric root");
        auto rad=solveBracketedRoot(Expression("sin(x)-0.5"),.1,1,AngleMode::Radians,empty);
        check(rad.converged()&&std::abs(rad.x-pi/6)<1e-7,"radian-mode trigonometric root");
        auto ans=solveBracketedRoot(Expression("x-ans"),0,4,AngleMode::Radians,empty,3.);
        check(ans.converged()&&std::abs(ans.x-3)<1e-7,"previous answer binding");
        auto searched=findRootsInDomain(f,-2,2,AngleMode::Radians,empty);
        check(searched.complete()&&searched.roots.size()==2,"compiled expression domain search");
    }
    {
        VariableScope scope;
        check(scope.declare("a"),"declare scalar a");
        Expression expr("x-a",scope);
        VariableBindings values;
        check(values.set("a",2),"bind scalar 2");
        auto first=solveBracketedRoot(expr,0,4,AngleMode::Radians,values);
        check(first.converged()&&std::abs(first.x-2)<1e-7,"snapshot initial binding");
        check(values.set("a",3),"bind scalar 3");
        auto second=solveBracketedRoot(expr,0,4,AngleMode::Radians,values);
        check(second.converged()&&std::abs(second.x-3)<1e-7,"snapshot updated binding");
        check(solveBracketedRoot(expr,0,4,AngleMode::Radians,VariableBindings{}).status==
              RootStatus::UndefinedEndpoint,"unbound scalar is undefined");
    }
    {
        FunctionDefinitions defs;
        check(defs.define("f(x)=x^2+1").accepted,"user function defined");
        VariableScope empty;
        Expression e("f(x)-5",empty,defs.scope());
        auto r=solveBracketedRoot(e,0,4,AngleMode::Radians,VariableBindings{});
        check(r.converged()&&std::abs(r.x-2)<1e-7,"compiled user function root");
    }
    {
        std::mt19937_64 rng(0x48B00AULL);
        std::uniform_real_distribution<double> targets(-5,5);
        for (int i=0;i<200;++i) {
            const double target=targets(rng);
            auto r=solveBracketedRoot([=](double x){return 3*(x-target);},-6,6);
            check(r.converged() && std::abs(r.x-target)<1e-7,
                  "fixed-seed affine target oracle");
            check(r.evaluations<=RootOptions{}.maxEvaluations,
                  "fixed-seed callback cap");
        }
    }
    check(std::string(rootMethodName(RootMethod::SafeguardedSecantBisection))==
          "safeguarded secant/bisection","method name");
    check(std::string(rootStatusName(RootStatus::NoSignChange))=="no sign change",
          "root status name");
    check(std::string(rootSearchStatusName(RootSearchStatus::IncompleteDomain))==
          "incomplete or undefined domain","search status name");
    std::cout<<(failures?"FAIL: ":"PASS: ")<<checks<<" NumericalRootFinding assertions\n";
    return failures?1:0;
}
