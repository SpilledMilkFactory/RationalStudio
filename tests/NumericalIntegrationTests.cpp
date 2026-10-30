#include "NumericalIntegration.hpp"
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
    void check(bool condition, const std::string& description) {
        ++checks;
        if (!condition) { ++failures; std::cerr << "FAIL: " << description << '\n'; }
    }
    void reference(const std::string& name, const IntegrationFunction& f,
                   double a, double b, double oracle, double tolerance = 1e-7) {
        const auto result = integrate(f,a,b);
        if (!result.converged())
            std::cerr << "INFO: " << name << " status=" << integrationStatusName(result.status)
            << " evaluations=" << result.evaluations
            << " splits=" << result.subdivisions << '\n';
        check(result.converged(), name + " converged");
        check(result.converged() && std::abs(result.value-oracle) <= tolerance,
              name + " analytical oracle");
        check(result.evaluations <= IntegrationOptions{}.maxEvaluations,
              name + " evaluation budget");
        check(result.converged() && std::isfinite(result.estimatedError),
              name + " finite error estimate");
                   }
}
int main() {
    const double pi = std::numbers::pi;
    reference("constant", [](double){return 4.;}, -2, 3, 20);
    reference("linear", [](double x){return 2*x+1;}, 0, 3, 12);
    reference("quadratic", [](double x){return x*x;}, 0, 3, 9);
    reference("cubic signed", [](double x){return x*x*x;}, -1, 2, 3.75);
    reference("sine half-wave", [](double x){return std::sin(x);}, 0, pi, 2);
    reference("cosine quarter", [](double x){return std::cos(x);}, 0, pi/2, 1);
    reference("exponential", [](double x){return std::exp(x);}, 0, 1, std::numbers::e-1);
    reference("logarithm", [](double x){return std::log(x);}, 1, std::numbers::e, 1);
    reference("reciprocal", [](double x){return 1/x;}, 1, 4, std::log(4));
    reference("absolute cusp", [](double x){return std::abs(x);}, -1, 1, 1);
    reference("reversed", [](double x){return x*x;}, 3, 0, -9);
    reference("oscillatory", [](double x){return std::sin(17*x);}, 0, 1,
              (1-std::cos(17.))/17, 2e-8);
    reference("step away from midpoint", [](double x){return x < .31 ? 1. : 2.;},
              0, 1, 1.69, 1e-7);
    reference("integrable endpoint singularity", [](double x){return 1/std::sqrt(x);},
              0, 1, 2., 4e-7);
    reference("narrow smooth peak", [](double x){return 1/(1+10000*(x-.43)*(x-.43));},
              0, 1, (std::atan(100*(1-.43))-std::atan(-43.))/100, 2e-7);
    {
        int n=0;
        const auto r=integrate([&](double){++n;return 42.;},2,2);
        check(r.converged()&&r.value==0&&r.evaluations==0&&n==0,
              "zero-width interval does not call integrand");
    }
    {
        const auto r=integrate([](double x){return x==0?std::numeric_limits<double>::quiet_NaN():x;},-1,1);
        check(r.status==IntegrationStatus::UndefinedIntegrand,"nonfinite midpoint rejected");
        check(!std::isfinite(r.value),"invalid sample never publishes a result");
    }
    {
        const auto r=integrate([](double){throw std::runtime_error("callback error");return 0.;},0,1);
        check(r.status==IntegrationStatus::UndefinedIntegrand,"throwing callback converted to domain failure");
    }
    {
        IntegrationOptions o;
        o.maxEvaluations=15;
        const auto r=integrate([](double x){return std::sin(17*x);},0,1,o);
        check(r.status==IntegrationStatus::EvaluationLimit,"evaluation-limit status");
        check(r.evaluations<=15,"strict callback ceiling");
        check(!std::isfinite(r.value),"budget-exhausted result not published");
    }
    {
        IntegrationOptions o;
        o.maxSubdivisions=1;
        const auto r=integrate([](double x){return std::sin(17*x);},0,1,o);
        check(r.status==IntegrationStatus::SubdivisionLimit,"subdivision-limit status");
        check(r.subdivisions==1,"exact subdivision ceiling");
    }
    {
        IntegrationOptions o;
        o.maxDepth=1;
        const auto r=integrate([](double x){return std::sin(17*x);},0,1,o);
        check(r.status==IntegrationStatus::Unresolved,"depth limit explicitly unresolved");
    }
    {
        const auto bad=std::numeric_limits<double>::infinity();
        check(integrate(IntegrationFunction{},0,1).status==IntegrationStatus::InvalidInput,"missing callback");
        check(integrate([](double x){return x;},0,bad).status==IntegrationStatus::InvalidInput,"nonfinite bounds");
        IntegrationOptions o; o.absoluteTolerance=-1;
        check(integrate([](double x){return x;},0,1,o).status==IntegrationStatus::InvalidInput,"invalid tolerance");
        o=IntegrationOptions{};o.maxEvaluations=14;
        check(integrate([](double x){return x;},0,1,o).status==IntegrationStatus::InvalidInput,"invalid budget");
        o=IntegrationOptions{};o.maxSubdivisions=0;
        check(integrate([](double x){return x;},0,1,o).status==IntegrationStatus::InvalidInput,"invalid subdivision cap");
        check(integrate([](double x){return x;},-1e308,1e308).status==IntegrationStatus::InvalidInput,"unrepresentable interval span");
    }
    {
        VariableBindings none;
        Expression cubic("x^3+2*x");
        const auto r=integrate(cubic,0,2,AngleMode::Radians,none);
        check(r.converged()&&std::abs(r.value-8)<1e-7,"compiled polynomial integration");
        Expression sine("sin(x)");
        const auto rad=integrate(sine,0,pi,AngleMode::Radians,none);
        const auto deg=integrate(sine,0,180,AngleMode::Degrees,none);
        check(rad.converged()&&std::abs(rad.value-2)<1e-7,"radian sine integral");
        check(deg.converged()&&std::abs(deg.value-360/pi)<2e-6,"degree sine integral");
        Expression invalid("unknown(x)");
        check(integrate(invalid,0,1,AngleMode::Radians,none).status==IntegrationStatus::InvalidInput,
              "invalid compiled expression rejected");
    }
    {
        VariableScope scope;
        check(scope.declare("a"),"scalar scope declaration");
        Expression source("a*x",scope);
        VariableBindings bindings;
        check(bindings.set("a",2),"bind coefficient 2");
        auto first=integrate(source,0,3,AngleMode::Radians,bindings);
        check(first.converged()&&std::abs(first.value-9)<1e-7,"first binding snapshot");
        check(bindings.set("a",4),"bind coefficient 4");
        auto second=integrate(source,0,3,AngleMode::Radians,bindings);
        check(second.converged()&&std::abs(second.value-18)<1e-7,"updated binding snapshot");
        VariableBindings missing;
        check(integrate(source,0,3,AngleMode::Radians,missing).status==IntegrationStatus::UndefinedIntegrand,
              "unbound variable rejected");
        Expression ans("ans*x");
        const auto ar=integrate(ans,0,2,AngleMode::Radians,missing,3.);
        check(ar.converged()&&std::abs(ar.value-6)<1e-7,"ans preserved");
    }
    {
        FunctionDefinitions defs;
        // A compiled user-function scope is captured by the expression.
        check(defs.define("f(x)=x^2+1").accepted,"user function defined");
        VariableScope variables;
        Expression expression("f(x)",variables,defs.scope());
        VariableBindings bindings;
        const auto r=integrate(expression,0,2,AngleMode::Radians,bindings);
        check(r.converged()&&std::abs(r.value-(8./3+2))<1e-7,
              "captured user function integral");
    }
    {
        std::mt19937_64 rng(0x47F17EULL);
        std::uniform_real_distribution<double> coords(-3,3);
        for(int i=0;i<120;++i) {
            const double a=coords(rng), b=coords(rng);
            const auto linear=integrate([](double x){return 3*x-2;},a,b);
            const double expected=1.5*(b*b-a*a)-2*(b-a);
            check(linear.converged()&&std::abs(linear.value-expected)<2e-7,
                  "fixed-seed affine integral oracle");
            const auto quadratic=integrate([](double x){return x*x+2*x+3;},a,b);
            const double expectedQ=(b*b*b-a*a*a)/3+(b*b-a*a)+3*(b-a);
            check(quadratic.converged()&&std::abs(quadratic.value-expectedQ)<2e-7,
                  "fixed-seed polynomial integral oracle");
        }
    }
    check(std::string(integrationMethodName(IntegrationMethod::GaussKronrod15))==
    "adaptive Gauss-Kronrod 7/15","method name");
    check(std::string(integrationStatusName(IntegrationStatus::SubdivisionLimit))==
    "subdivision limit","status name");
    std::cout << (failures ? "FAIL: " : "PASS: ") << checks
    << " NumericalIntegration assertions\n";
    return failures ? 1 : 0;
}
