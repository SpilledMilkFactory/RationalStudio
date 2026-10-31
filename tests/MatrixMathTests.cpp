#include "MatrixMath.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

using namespace rationalstudio;
namespace {
int checks=0,failures=0;
void check(bool yes,const std::string& label) {
    ++checks;
    if (!yes) {++failures;std::cerr<<"FAIL: "<<label<<'\n';}
}
bool near(double actual,double expected,double tolerance=1e-10) {
    return std::isfinite(actual) && std::abs(actual-expected)<=tolerance*(1.0+std::abs(expected));
}
bool nearVector(const MathVector& actual,const MathVector& expected,double tol=1e-10) {
    if (actual.size()!=expected.size()) return false;
    for (std::size_t i=0;i<actual.size();++i)
        if (!near(actual.values[i],expected.values[i],tol)) return false;
    return true;
}
}
int main() {
    const auto a=makeMatrix(2,2,{1,2,3,4});
    const auto b=makeMatrix(2,2,{5,6,7,8});
    const auto v=makeVector({2,-1});
    check(a.successful()&&b.successful()&&v.successful(),"basic constructors");
    check(a.successful()&&a.value.at(1,0)==3,"row-major indexing");
    const auto sum=addMatrices(a.value,b.value);
    check(sum.successful()&&sum.value.values==std::vector<double>({6,8,10,12}),"matrix addition");
    const auto difference=subtractMatrices(b.value,a.value);
    check(difference.successful()&&difference.value.values==std::vector<double>({4,4,4,4}),"matrix subtraction");
    const auto scaled=scaleMatrix(a.value,-2);
    check(scaled.successful()&&scaled.value.values==std::vector<double>({-2,-4,-6,-8}),"matrix scaling");
    const auto product=multiplyMatrices(a.value,b.value);
    check(product.successful()&&product.value.values==std::vector<double>({19,22,43,50}),"matrix multiplication");
    const auto trans=transposeMatrix(a.value);
    check(trans.successful()&&trans.value.values==std::vector<double>({1,3,2,4}),"matrix transpose");
    const auto av=multiplyMatrixVector(a.value,v.value);
    check(av.successful()&&av.value.values==std::vector<double>({0,2}),"matrix-vector multiplication");
    const auto vv=addVectors(v.value,makeVector({-2,3}).value);
    check(vv.successful()&&vv.value.values==std::vector<double>({0,2}),"vector addition");
    const auto vd=subtractVectors(v.value,makeVector({-2,3}).value);
    check(vd.successful()&&vd.value.values==std::vector<double>({4,-4}),"vector subtraction");
    const auto vs=scaleVector(v.value,3);
    check(vs.successful()&&vs.value.values==std::vector<double>({6,-3}),"vector scaling");
    const auto dot=dotVectors(v.value,makeVector({3,5}).value);
    check(dot.successful()&&dot.value==1,"vector dot product");
    check(near(normVector(makeVector({3,4}).value).value,5),"vector Euclidean norm");
    check(determinant(a.value).successful()&&near(determinant(a.value).value,-2),"determinant 2x2");
    check(determinant(makeMatrix(2,2,{1,2,2,4}).value).value==0,"singular determinant");
    const auto rect=makeMatrix(2,3,{1,2,3,4,5,6});
    const auto rtrans=transposeMatrix(rect.value);
    check(rect.successful()&&rtrans.successful()&&rtrans.value.rows==3&&rtrans.value.columns==2&&
          rtrans.value.values==std::vector<double>({1,4,2,5,3,6}),"rectangular transpose");
    const auto m31=makeMatrix(3,1,{2,3,4});
    const auto rm=multiplyMatrices(rect.value,m31.value);
    check(rm.successful()&&rm.value.values==std::vector<double>({20,47}),"rectangular multiply");

    const auto system=makeMatrix(3,3,{3,2,-1,2,-2,4,-1,0.5,-1});
    const auto rhs=makeVector({1,-2,0});
    const auto solution=solveLinearSystem(system.value,rhs.value);
    check(solution.successful(),"pivoted 3x3 solve");
    check(solution.successful()&&nearVector(solution.solution,makeVector({1,-2,-2}).value),"3x3 oracle");
    check(solution.successful()&&solution.relativeResidual<1e-12,"3x3 residual");
    check(solution.successful()&&solution.reciprocalCondition>0,"rcond reported");
    const auto swap=solveLinearSystem(makeMatrix(2,2,{0,1,1,1}).value,
                                      makeVector({2,3}).value);
    check(swap.successful()&&nearVector(swap.solution,makeVector({1,2}).value),"row pivoting");
    const auto singular=solveLinearSystem(makeMatrix(2,2,{1,2,2,4}).value,makeVector({3,6}).value);
    check(singular.status==MatrixStatus::Singular&&singular.solution.values.empty(),"singular solve withheld");
    const auto ill=solveLinearSystem(makeMatrix(2,2,{1,1,1,1+1e-14}).value,
                                     makeVector({2,2+1e-14}).value);
    check(ill.status==MatrixStatus::IllConditioned&&ill.solution.values.empty(),"ill conditioned rejected");
    LinearSolveOptions relaxed;
    relaxed.minReciprocalCondition=1e-16;
    const auto relaxedSolve=solveLinearSystem(makeMatrix(2,2,{1,1,1,1+1e-14}).value,
                                              makeVector({2,2+1e-14}).value,relaxed);
    check(relaxedSolve.successful()&&relaxedSolve.reciprocalCondition<1e-12,
          "explicit relaxed condition threshold");

    const auto missing=makeMatrix(2,3,{1,2,3});
    check(missing.status==MatrixStatus::InvalidShape,"missing matrix entries rejected");
    check(makeMatrix(0,1,{}).status==MatrixStatus::InvalidShape,"zero dimension rejected");
    check(makeMatrix(129,1,std::vector<double>(129,1)).status==MatrixStatus::InvalidShape,"dimension cap");
    check(makeMatrix(128,128,std::vector<double>(16384,1)).successful(),"maximum matrix size");
    check(makeMatrix(128,129,std::vector<double>(16512,1)).status==MatrixStatus::InvalidShape,"max product bound");
    check(makeVector({}).status==MatrixStatus::InvalidShape,"empty vector rejected");
    check(makeMatrix(1,1,{std::numeric_limits<double>::infinity()}).status==MatrixStatus::NonFiniteInput,
          "nonfinite matrix input");
    check(makeVector({std::numeric_limits<double>::quiet_NaN()}).status==MatrixStatus::NonFiniteInput,
          "nonfinite vector input");
    check(addMatrices(rect.value,a.value).status==MatrixStatus::DimensionMismatch,"addition mismatch");
    check(multiplyMatrices(a.value,rect.value).successful(),"legal dimension multiplication");
    check(multiplyMatrices(rect.value,a.value).status==MatrixStatus::DimensionMismatch,"multiplication mismatch");
    check(multiplyMatrixVector(rect.value,v.value).status==MatrixStatus::DimensionMismatch,"matrix vector mismatch");
    check(dotVectors(v.value,makeVector({1}).value).status==MatrixStatus::DimensionMismatch,"dot mismatch");
    check(determinant(rect.value).status==MatrixStatus::InvalidShape,"non-square determinant");
    check(solveLinearSystem(rect.value,v.value).status==MatrixStatus::InvalidShape,"non-square solve");
    check(solveLinearSystem(a.value,makeVector({1}).value).status==MatrixStatus::DimensionMismatch,"rhs size mismatch");
    check(scaleMatrix(a.value,std::numeric_limits<double>::infinity()).status==MatrixStatus::NonFiniteInput,
          "invalid scale factor");
    check(scaleMatrix(makeMatrix(1,1,{1e308}).value,2).status==MatrixStatus::NonFiniteResult,
          "matrix overflow rejected");
    check(dotVectors(makeVector({1e308}).value,makeVector({1e308}).value).status==MatrixStatus::NonFiniteResult,
          "dot overflow rejected");
    check(normVector(makeVector({1e308,1e308}).value).successful(),"stable norm near huge magnitudes");
    check(determinant(makeMatrix(2,2,{1e308,0,0,1e308}).value).status==MatrixStatus::NonFiniteResult,
          "det overflow rejected");
    LinearSolveOptions invalid;
    invalid.minReciprocalCondition=0;
    check(solveLinearSystem(a.value,v.value,invalid).status==MatrixStatus::InvalidShape,"invalid rcond threshold");
    invalid=LinearSolveOptions{};invalid.relativeResidualTolerance=-1;
    check(solveLinearSystem(a.value,v.value,invalid).status==MatrixStatus::InvalidShape,"invalid residual threshold");
    invalid=LinearSolveOptions{};invalid.maxRefinements=9;
    check(solveLinearSystem(a.value,v.value,invalid).status==MatrixStatus::InvalidShape,"invalid refinement cap");
    check(std::string(matrixStatusName(MatrixStatus::IllConditioned))=="ill-conditioned matrix",
          "status string");
    check(!solveLinearSystem(makeMatrix(1,1,{0}).value,makeVector({0}).value).successful(),"zero matrix rejected");
    check(solveLinearSystem(makeMatrix(1,1,{2}).value,makeVector({10}).value).successful(),"scalar system");

    // Deterministic, seeded systems with diagonally dominant coefficients.
    // Reference vectors are known independently, not derived from the solve.
    std::mt19937_64 random(0x5050AA55ULL);
    std::uniform_int_distribution<int> entries(-4,4);
    for (int trial=0;trial<160;++trial) {
        std::vector<double> data(9),x(3),bvec(3);
        for (double& item:data) item=entries(random);
        for (int i=0;i<3;++i) data[i*3+i]+=25;
        for (double& item:x) item=entries(random);
        for (int row=0;row<3;++row)
            for (int col=0;col<3;++col) bvec[row]+=data[row*3+col]*x[col];
        auto matrix=makeMatrix(3,3,data);
        auto result=solveLinearSystem(matrix.value,makeVector(bvec).value);
        check(result.successful(),"seeded 3x3 convergence");
        check(result.successful()&&nearVector(result.solution,makeVector(x).value,2e-11),
              "seeded analytical solution");
        check(result.successful()&&result.relativeResidual<=1e-10,"seeded numerical residual");
        auto identity=makeMatrix(3,3,{1,0,0,0,1,0,0,0,1});
        auto left=multiplyMatrices(matrix.value,identity.value);
        check(left.successful()&&left.value.values==data,"seeded right identity");
    }
    std::cout<<(failures?"FAIL: ":"PASS: ")<<checks<<" MatrixMath assertions\n";
    return failures?1:0;
}
