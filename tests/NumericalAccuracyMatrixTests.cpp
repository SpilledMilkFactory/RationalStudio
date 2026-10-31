// M4-09 / Issue #54: deterministic cross-service accuracy oracles.
// Deliberately additive: no production algorithm or GUI changes.
#include "NumericalDifferentiation.hpp"
#include "NumericalIntegration.hpp"
#include "NumericalRootFinding.hpp"
#include "ComplexMath.hpp"
#include "MatrixMath.hpp"
#include "DescriptiveStatistics.hpp"
#include "StatisticalRegression.hpp"
#include "AdvancedMathInput.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numbers>
#include <random>
#include <string>
#include <variant>
#include <vector>
using namespace rationalstudio;
namespace {
int assertions = 0, failures = 0;
void check(bool good, const std::string& label) {
    ++assertions;
    if (!good) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool near(double actual, double expected, double absolute = 1e-8, double relative = 1e-8) {
    return std::isfinite(actual) && std::isfinite(expected) &&
           std::abs(actual - expected) <= absolute + relative * std::abs(expected);
}
void polynomialOracle(double a, double b, double left, double right, int index) {
    // f(x)=a*x+b: integration, differentiation, and root solving have exact oracles.
    const auto f = [=](double x) { return a*x+b; };
    const auto d = differentiate(f, 0.375);
    check(d.converged(), "affine derivative converged " + std::to_string(index));
    check(d.converged() && near(d.value, a, 1e-7, 1e-7), "affine derivative oracle " + std::to_string(index));
    check(d.evaluations <= DifferentiationOptions{}.maxEvaluations, "derivative budget " + std::to_string(index));
    const auto integral = integrate(f,left,right);
    const double oracle = .5*a*(right*right-left*left) + b*(right-left);
    check(integral.converged(), "affine quadrature converged " + std::to_string(index));
    check(integral.converged() && near(integral.value,oracle,1e-8,1e-8),"affine integral oracle " + std::to_string(index));
    check(integral.evaluations<=IntegrationOptions{}.maxEvaluations,"quadrature budget " + std::to_string(index));
    const double root = -b/a;
    const auto result=solveBracketedRoot(f,root-2.,root+2.);
    check(result.converged(),"affine root converged " + std::to_string(index));
    check(result.converged() && near(result.x,root,1e-8,1e-8),"affine root oracle " + std::to_string(index));
    check(result.converged() && result.residual<=RootOptions{}.residualTolerance,"root residual " + std::to_string(index));
}
}
int main() {
    constexpr double pi = std::numbers::pi;
    // Cross-check differentiation and integration of the same compiled expression.
    const Expression cubic("x^3 - 2*x + 5");
    const VariableBindings empty;
    check(cubic.valid(),"compiled polynomial accepted");
    for (double x : {-2.,-1.,0.,0.5,1.,2.}) {
        const auto d=differentiate(cubic,x,AngleMode::Radians,empty);
        check(d.converged(),"compiled cubic derivative status");
        check(d.converged() && near(d.value,3*x*x-2,3e-6,2e-6),"compiled cubic derivative oracle");
    }
    const auto area=integrate(cubic,-1.,2.,AngleMode::Radians,empty);
    // F(x)=x^4/4-x^2+5x, F(2)=10, F(-1)=-5.75
    check(area.converged(),"compiled cubic integral status");
    check(area.converged() && near(area.value,15.75),"compiled cubic integral oracle");
    const Expression rootExpr("x^2-2");
    const auto root=solveBracketedRoot(rootExpr,0,2,AngleMode::Radians,empty);
    check(root.converged(),"compiled sqrt2 root status");
    check(root.converged() && near(root.x,std::sqrt(2.),1e-9,1e-9),"compiled sqrt2 root oracle");
    const Expression radSin("sin(x)");
    const auto rad=integrate(radSin,0,pi,AngleMode::Radians,empty);
    const auto deg=integrate(radSin,0,180.,AngleMode::Degrees,empty);
    check(rad.converged()&&near(rad.value,2.,1e-7,1e-7),"radian sine integral");
    check(deg.converged()&&near(deg.value,360./pi,1e-6,1e-7),"degree sine integral");

    // Complex principal branches, elementary identities, typed isolation.
    for (const auto& [formula, expected] : std::vector<std::pair<std::string,ComplexValue>>{
         {"sqrt(-1)",{0,1}}, {"ln(-1)",{0,pi}}, {"(2+3i)*(2-3i)",{13,0}},
         {"exp(i*pi)",{-1,0}}, {"sin(i)",{0,std::sinh(1.)}},
         {"(1+i)^2",{0,2}}, {"abs(3+4i)",{5,0}}, {"conj(3+4i)",{3,-4}}}) {
        ComplexExpression c(formula);
        check(c.valid(),"complex parse: "+formula);
        const auto result=c.evaluate();
        check(result.successful(),"complex execution: "+formula);
        check(result.successful() && std::abs(result.value-expected)<1e-9,
              "complex reference: "+formula);
    }
    const auto promoted=evaluateComplex(Expression("3*x+2"),4,0,AngleMode::Radians,empty);
    check(promoted.successful() && std::abs(promoted.value-ComplexValue(14,0))<1e-12,
          "legacy real expression promoted without complex grammar");

    // Matrix identities, checked linear solver and vector dot/norm.
    const auto A=makeMatrix(2,2,{3,1,1,2});
    const auto B=makeMatrix(2,2,{2,0,0,2});
    const auto rhs=makeVector({9,8});
    check(A.successful()&&B.successful()&&rhs.successful(),"linear algebra fixtures");
    const auto matrixProduct=multiplyMatrices(A.value,B.value);
    check(matrixProduct.successful() && matrixProduct.value.values==std::vector<double>({6,2,2,4}),
          "matrix product oracle");
    const auto linear=solveLinearSystem(A.value,rhs.value);
    check(linear.successful(),"pivoted solution status");
    check(linear.successful()&&near(linear.solution.values[0],2)&&near(linear.solution.values[1],3),
          "pivoted solution known vector");
    check(linear.successful()&&linear.relativeResidual<1e-12,"linear backward residual");
    const auto det=determinant(A.value);
    check(det.successful()&&near(det.value,5),"determinant oracle");
    const auto norm=normVector(makeVector({3,4}).value);
    check(norm.successful()&&near(norm.value,5),"vector norm oracle");

    // Welford and QR services should agree with independent analytical data.
    const std::vector<double> xs{1,2,3,4,5};
    const std::vector<double> ys{3,5,7,9,11};
    const auto stats=summarizeStatistics(ys);
    check(stats.successful(),"statistics status");
    check(stats.successful()&&near(stats.mean,7),"statistics mean oracle");
    check(stats.successful()&&near(stats.populationVariance,8),"population variance oracle");
    check(stats.successful()&&near(stats.sampleVariance,10),"sample variance oracle");
    const auto fit=fitLinearRegression(xs,ys);
    const auto coeff=linearRegressionCoefficients(fit);
    check(fit.successful()&&coeff.successful(),"linear regression status");
    check(coeff.successful()&&near(coeff.intercept,1)&&near(coeff.slope,2),"linear regression coefficients");
    check(fit.successful()&&near(fit.rSquared,1),"linear regression R2");
    const auto pred=predictRegression(fit,6);
    check(pred.successful()&&near(pred.value,13),"linear prediction oracle");
    const auto data=parseAdvancedNumbers("3,5,7,9,11");
    check(data.valid()&&data.values==ys,"typed input preserves statistical data");
    const AdvancedMathValue typed=fit;
    check(std::string(advancedValueType(typed))=="regression","typed result preserved");

    // Stable fixed-seed reference matrix deliberately uses small values and
    // distinct brackets; it tests independent algorithms with known formulas.
    std::mt19937_64 rng(0x54A11CEULL);
    std::uniform_real_distribution<double> coefficients(-4,4), abscissas(-2,2);
    for (int i=0;i<80;++i) {
        double slope=coefficients(rng);
        if (std::abs(slope)<0.5) slope=slope<0?-0.5:0.5;
        const double offset=coefficients(rng), l=abscissas(rng), r=abscissas(rng);
        polynomialOracle(slope,offset,l,r,i);
    }
    std::cout << (failures ? "FAIL: " : "PASS: ") << assertions
              << " NumericalAccuracyMatrix assertions\n";
    return failures ? 1 : 0;
}
