#include "NumericalDifferentiation.hpp"
#include "FunctionDefinitions.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <random>
#include <stdexcept>
#include <string>

using namespace rationalstudio;
namespace {
int checks=0, failures=0;
void check(bool ok, const std::string& reason) {
    ++checks;
    if(!ok){++failures;std::cerr<<"FAIL: "<<reason<<'\n';}
}
void derivative(const std::string& label, const DifferentiationFunction& f,
                double x, double target, double tolerance,
                DerivativeMethod expected=DerivativeMethod::CentralRichardson) {
    auto r=differentiate(f,x);
    if(!r.converged())
        std::cerr<<"INFO: "<<label<<" status="<<derivativeStatusName(r.status)
                 <<" evaluations="<<r.evaluations<<" refinements="<<r.refinements<<'\n';
    check(r.converged(),label+" converges");
    check(r.converged() && std::abs(r.value-target)<=tolerance,label+" matches analytic derivative");
    check(r.method==expected,label+" uses expected stencil");
    check(r.evaluations<=DifferentiationOptions{}.maxEvaluations,label+" bounded evaluations");
    check(r.converged() && std::isfinite(r.estimatedError) && r.step>0,label+" diagnostics finite");
}
}
int main() {
    derivative("constant",[](double){return 4.;},0,0,1e-12);
    derivative("line",[](double x){return 5*x-2;},2,5,1e-8);
    derivative("quadratic",[](double x){return x*x;},3,6,1e-8);
    derivative("cubic",[](double x){return x*x*x;},2,12,1e-7);
    derivative("quartic flat",[](double x){return x*x*x*x;},0,0,1e-9);
    derivative("sine",[](double x){return std::sin(x);},0.8,std::cos(0.8),2e-7);
    derivative("cosine",[](double x){return std::cos(x);},0.8,-std::sin(0.8),2e-7);
    derivative("exponential",[](double x){return std::exp(x);},1,std::numbers::e,5e-6);
    derivative("log",[](double x){return std::log(x);},2,0.5,2e-7);
    derivative("reciprocal",[](double x){return 1/x;},2,-0.25,2e-7);
    derivative("smooth abs",[](double x){return std::abs(x);},2,1,1e-8);
    derivative("one-sided positive",[](double x){return x<0?std::numeric_limits<double>::quiet_NaN():x*x;},0,0,1e-7,DerivativeMethod::ForwardRichardson);
    derivative("one-sided negative",[](double x){return x>0?std::numeric_limits<double>::quiet_NaN():x*x;},0,0,1e-7,DerivativeMethod::BackwardRichardson);

    {
        const auto r=differentiate([](double x){return std::abs(x);},0);
        check(!r.converged()&&r.status==DerivativeStatus::NonDifferentiable,"absolute-value cusp rejected");
        check(!std::isfinite(r.value),"cusp does not publish estimate");
    }
    {
        const auto r=differentiate([](double x){return x<0?0.:1.;},0);
        check(!r.converged(),"jump discontinuity rejected");
    }
    {
        const auto r=differentiate([](double x){return x<0?std::numeric_limits<double>::quiet_NaN():std::sqrt(x);},0);
        check(!r.converged(),"unbounded endpoint derivative rejected");
    }
    {
        const auto r=differentiate([](double x){return std::log(x);},0);
        check(r.status==DerivativeStatus::UndefinedAtPoint,"undefined base point rejected");
    }
    {
        const auto r=differentiate([](double x)->double{if(x!=0)throw std::runtime_error("invalid");return 1;},0);
        check(r.status==DerivativeStatus::NoFiniteStencil,"throwing callback treated as invalid stencil");
    }
    {
        DifferentiationOptions o;
        o.maxEvaluations=5;
        const auto r=differentiate([](double x){return std::sin(x);},1,o);
        check(r.status==DerivativeStatus::EvaluationLimit,"budget limit explicit");
        check(r.evaluations<=5,"budget never exceeded");
    }
    {
        DifferentiationOptions o;
        o.allowOneSided=false;
        auto r=differentiate([](double x){return x<0?std::numeric_limits<double>::quiet_NaN():x*x;},0,o);
        check(r.status==DerivativeStatus::NoFiniteStencil,"one-sided fallback can be disabled");
    }
    {
        DifferentiationOptions o;
        check(differentiate(DifferentiationFunction{},0).status==DerivativeStatus::InvalidInput,
              "empty callback rejected");
        check(differentiate([](double x){return x;},std::numeric_limits<double>::infinity()).status==DerivativeStatus::InvalidInput,
              "nonfinite abscissa rejected");
        o.initialRelativeStep=0;
        check(differentiate([](double x){return x;},0,o).status==DerivativeStatus::InvalidInput,
              "invalid step rejected");
        o=DifferentiationOptions{};o.relativeTolerance=std::numeric_limits<double>::quiet_NaN();
        check(differentiate([](double x){return x;},0,o).status==DerivativeStatus::InvalidInput,
              "invalid tolerance rejected");
        o=DifferentiationOptions{};o.maxRefinements=2;
        check(differentiate([](double x){return x;},0,o).status==DerivativeStatus::InvalidInput,
              "invalid iteration bound rejected");
    }
    {
        VariableBindings none;
        Expression polynomial("x^3+2*x");
        const auto r=differentiate(polynomial,2,AngleMode::Radians,none);
        check(r.converged()&&std::abs(r.value-14)<1e-6,"compiled expression cubic derivative");
        Expression sine("sin(x)");
        const auto rad=differentiate(sine,0,AngleMode::Radians,none);
        const auto deg=differentiate(sine,0,AngleMode::Degrees,none);
        check(rad.converged()&&std::abs(rad.value-1)<1e-7,"radian mode derivative");
        check(deg.converged()&&std::abs(deg.value-std::numbers::pi/180)<1e-7,"degree mode derivative");
        Expression invalid("unknown(x)");
        check(differentiate(invalid,0,AngleMode::Radians,none).status==DerivativeStatus::InvalidInput,
              "invalid compiled expression rejected");
    }
    {
        VariableScope scope;
        check(scope.declare("a"),"scalar scope created");
        Expression bound("a*x*x",scope);
        VariableBindings b;check(b.set("a",3),"bind coefficient 3");
        const auto first=differentiate(bound,2,AngleMode::Radians,b);
        check(first.converged()&&std::abs(first.value-12)<1e-6,"binding snapshot A");
        check(b.set("a",5),"bind coefficient 5");
        const auto second=differentiate(bound,2,AngleMode::Radians,b);
        check(second.converged()&&std::abs(second.value-20)<1e-6,"binding snapshot B");
        VariableBindings unbound;
        check(differentiate(bound,2,AngleMode::Radians,unbound).status==DerivativeStatus::UndefinedAtPoint,
              "unbound scalar does not return derivative");
    }
    {
        Expression ans("ans*x");
        VariableBindings none;
        const auto r=differentiate(ans,2,AngleMode::Radians,none,7);
        check(r.converged()&&std::abs(r.value-7)<1e-6,"ans fixed over differentiation");
    }
    {
        FunctionDefinitions functions;
        check(functions.define("f(t)=t*t+1").accepted,"function definition accepted");
        check(functions.define("g(u)=f(u)*u").accepted,"nested function accepted");
        Expression nested("g(x)",{},functions.scope());
        VariableBindings none;
        const auto r=differentiate(nested,2,AngleMode::Radians,none);
        check(r.converged()&&std::abs(r.value-13)<1e-5,"nested user function derivative");
        check(functions.define("f(t)=t*t*t").accepted,"redefinition accepted");
        const auto unchanged=differentiate(nested,2,AngleMode::Radians,none);
        check(unchanged.converged()&&std::abs(unchanged.value-13)<1e-5,"immutable compiled function snapshot");
        Expression updated("g(x)",{},functions.scope());
        const auto newer=differentiate(updated,2,AngleMode::Radians,none);
        check(newer.converged()&&std::abs(newer.value-32)<1e-5,"recompiled function derivative");
    }
    {
        // Reproducible analytical matrix; this is a numeric reference check,
        // not a proof of correctness for arbitrary expressions.
        std::mt19937_64 rng(0x46D1FFULL);
        std::uniform_real_distribution<double> points(-4.0,4.0);
        for(int i=0;i<120;++i) {
            const double x=points(rng);
            const auto quadratic=differentiate([](double t){return t*t+3*t-7;},x);
            check(quadratic.converged()&&std::abs(quadratic.value-(2*x+3))<5e-6,
                  "fixed-seed quadratic oracle");
            const auto sine=differentiate([](double t){return std::sin(t);},x);
            check(sine.converged()&&std::abs(sine.value-std::cos(x))<5e-6,
                  "fixed-seed trigonometric oracle");
        }
    }
    check(std::string(derivativeMethodName(DerivativeMethod::CentralRichardson))=="central Richardson",
          "stable method name");
    check(std::string(derivativeStatusName(DerivativeStatus::Unstable))=="unstable or unresolved",
          "stable status name");
    std::cout<<(failures?"FAIL: ":"PASS: ")<<checks<<" NumericalDifferentiation assertions\n";
    return failures?1:0;
}
