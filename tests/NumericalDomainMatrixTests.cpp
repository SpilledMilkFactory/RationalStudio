// M4-09 / Issue #54: cross-service rejection and bounded-work oracles.
#include "NumericalDifferentiation.hpp"
#include "NumericalIntegration.hpp"
#include "NumericalRootFinding.hpp"
#include "ComplexMath.hpp"
#include "MatrixMath.hpp"
#include "DescriptiveStatistics.hpp"
#include "StatisticalRegression.hpp"
#include "AdvancedMathInput.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
using namespace rationalstudio;
namespace {
int assertions=0, failures=0;
void check(bool ok, const std::string& label) {
    ++assertions;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
}
int main() {
    const double inf=std::numeric_limits<double>::infinity();
    const double nan=std::numeric_limits<double>::quiet_NaN();
    // Differentiation: domain/non-smooth detection and strict callback limits.
    check(differentiate([](double x){return std::abs(x);},0).status==DerivativeStatus::NonDifferentiable,
          "cusp flagged without fabricated derivative");
    check(differentiate([](double x){return std::log(x);},0).status==DerivativeStatus::UndefinedAtPoint,
          "undefined base point");
    check(differentiate([](double x){return x;},inf).status==DerivativeStatus::InvalidInput,
          "derivative nonfinite coordinate");
    DifferentiationOptions dopt;
    dopt.maxEvaluations=5;
    const auto boundedDerivative=differentiate([](double x){return std::sin(x);},1,dopt);
    check(boundedDerivative.evaluations<=5,"derivative callback cap");
    check(boundedDerivative.status==DerivativeStatus::EvaluationLimit,"derivative budget status");
    check(!std::isfinite(boundedDerivative.value),"failed derivative not published");

    // Quadrature: bad samples, finite bounds and hard evaluation ceiling.
    check(integrate([](double x){return x;},0,inf).status==IntegrationStatus::InvalidInput,
          "quadrature rejects infinite bounds");
    const auto midpoint=integrate([&](double x){return x==0?nan:x;},-1,1);
    check(midpoint.status==IntegrationStatus::UndefinedIntegrand,"quadrature invalid midpoint");
    check(!std::isfinite(midpoint.value),"invalid quadrature never published");
    IntegrationOptions iopt;
    iopt.maxEvaluations=15;
    const auto boundedIntegral=integrate([](double x){return std::sin(17*x);},0,1,iopt);
    check(boundedIntegral.evaluations<=15,"quadrature evaluation ceiling");
    check(boundedIntegral.status==IntegrationStatus::EvaluationLimit,"quadrature budget status");
    check(!std::isfinite(boundedIntegral.value),"budget-exhausted area withheld");
    std::size_t calls=0;
    const auto zero=integrate([&](double){++calls;return 1.;},5,5);
    check(zero.converged()&&zero.value==0&&calls==0,"empty interval needs no sample");

    // Root solver: a sign flip is NOT continuity or proof of a root.
    check(solveBracketedRoot([](double x){return x*x+1;},-1,1).status==RootStatus::NoSignChange,
          "no bracketed sign change");
    const auto jump=solveBracketedRoot([](double x){return x<0?-1.:1.;},-1,1);
    check(!jump.converged(),"jump not certified root");
    check(!std::isfinite(jump.x),"jump does not publish a root");
    check(solveBracketedRoot([](double x){return x;},0,inf).status==RootStatus::InvalidInput,
          "root solver rejects infinite endpoints");
    RootOptions ropt;
    ropt.maxEvaluations=2;
    const auto limited=solveBracketedRoot([](double x){return x*x-2;},0,2,ropt);
    check(limited.evaluations<=2,"root solver callback ceiling");
    check(!limited.converged(),"insufficient root budget not success");

    // Complex: invalid operations and principal-branch domain boundaries.
    for (const char* s : {"1/0","ln(0)","0^0","exp(1000)"}) {
        ComplexExpression expression(s);
        check(expression.valid(),std::string("complex runtime case parsed: ")+s);
        const auto r=expression.evaluate();
        check(r.status==ComplexStatus::UndefinedValue,std::string("complex undefined: ")+s);
    }
    check(!ComplexExpression("unknown(2)").valid(),"invalid complex syntax");
    check(formatComplex({inf,0})=="undefined","complex formatting nonfinite");

    // Linear algebra: badly conditioned and nonfinite results never escape.
    const auto singularA=makeMatrix(2,2,{1,2,2,4});
    const auto singularB=makeVector({3,6});
    const auto singular=solveLinearSystem(singularA.value,singularB.value);
    check(singular.status==MatrixStatus::Singular,"singular matrix status");
    check(singular.solution.values.empty(),"singular solve withholds solution");
    const auto ill=solveLinearSystem(makeMatrix(2,2,{1,1,1,1+1e-14}).value,
                                     makeVector({2,2+1e-14}).value);
    check(ill.status==MatrixStatus::IllConditioned,"ill-conditioned solve flagged");
    check(ill.solution.values.empty(),"ill-conditioned solve withheld");
    check(makeMatrix(2,2,{1,nan,2,3}).status==MatrixStatus::NonFiniteInput,
          "nonfinite matrix input");
    check(multiplyMatrices(makeMatrix(1,1,{1e308}).value,
                           makeMatrix(1,1,{1e308}).value).status==MatrixStatus::NonFiniteResult,
          "matrix multiplication overflow");
    check(addMatrices(makeMatrix(1,2,{1,2}).value,makeMatrix(2,1,{1,2}).value).status==
          MatrixStatus::DimensionMismatch,"matrix dimensions checked");

    // Descriptive stats and QR regression: explicit domain/rank failures.
    const std::vector<double> none;
    check(summarizeStatistics(none).status==StatisticsStatus::EmptyInput,"empty descriptive data");
    check(summarizeStatistics(std::vector<double>{1,nan}).status==StatisticsStatus::NonFiniteInput,
          "nonfinite descriptive data");
    const auto singleton=summarizeStatistics(std::vector<double>{3});
    check(singleton.successful()&&!singleton.hasSampleVariance(),"singleton sample variance undefined");
    check(singleton.successful() && std::isnan(singleton.sampleVariance),"no fabricated sample variance");
    check(calculatePercentile(std::vector<double>{1,2,3},101).status==StatisticsStatus::InvalidPercentile,
          "invalid percentile rejected");
    const std::vector<double> constantX{2,2,2,2};
    const std::vector<double> changingY{1,2,3,4};
    check(fitLinearRegression(constantX,changingY).status==RegressionStatus::RankDeficient,
          "rank-deficient regression");
    check(fitLinearRegression(std::vector<double>{1,2},std::vector<double>{1}).status==
          RegressionStatus::SizeMismatch,"regression size mismatch");
    check(fitPolynomialRegression(std::vector<double>{0,1},std::vector<double>{1,3},4).status==
          RegressionStatus::TooFewSamples,"regression insufficient observations");
    check(!predictRegression(RegressionResult{},1).successful(),"uninitialized regression no prediction");

    // Strict presentation input parsing and typed transport for all result kinds.
    for(const auto& value : {"","1,","1,,2","1garbage","nan","inf"})
        check(!parseAdvancedNumbers(value).valid(),std::string("invalid numeric list: ")+value);
    for(const auto& value : {"1,2;3","1,2;","1,2;;3,4","1,2;3,inf"})
        check(!parseAdvancedMatrix(value).valid(),std::string("invalid matrix: ")+value);
    check(!parseAdvancedNumbers("1,2,3",2).valid(),"input size limit enforced");

    // Repeatability: invalid input never mutates a compiled expression.
    const Expression safe("x^2+1");
    const VariableBindings binding;
    for (int j=0;j<40;++j) {
        check(solveBracketedRoot(safe,-1,1,AngleMode::Radians,binding).status==
              RootStatus::NoSignChange,"repeat no real root");
        check(integrate(safe,-1,1,AngleMode::Radians,binding).converged(),
              "compiled source stable after failed root search");
    }
    std::cout << (failures ? "FAIL: " : "PASS: ") << assertions
              << " NumericalDomainMatrix assertions\n";
    return failures ? 1 : 0;
}
