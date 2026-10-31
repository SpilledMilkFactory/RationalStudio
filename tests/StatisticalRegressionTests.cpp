#include "StatisticalRegression.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

using namespace rationalstudio;
namespace {
int assertions=0, failures=0;
void check(bool condition, const std::string& label) {
    ++assertions;
    if (!condition) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool near(double actual, double expected, double tolerance=1e-8) {
    return std::isfinite(actual) && std::isfinite(expected) &&
        std::abs(actual-expected) <= tolerance * std::max(1.0, std::abs(expected));
}
void checkPred(const RegressionResult& model, double x, double y, double tolerance=1e-8) {
    const auto p=predictRegression(model,x);
    check(p.successful() && near(p.value,y,tolerance),"prediction reference");
}
}
int main() {
    {
        const std::vector<double> x{-2,-1,0,1,2,3};
        const std::vector<double> y{-3,-1,1,3,5,7};
        const auto model=fitLinearRegression(x,y);
        check(model.successful(),"linear model accepted");
        check(model.degree==1 && model.observations==6,"linear model metadata");
        const auto standard=linearRegressionCoefficients(model);
        check(standard.successful() && near(standard.slope,2) &&
              near(standard.intercept,1),"conventional slope/intercept");
        check(near(model.rSquared,1),"linear r2 one");
        check(near(model.pearsonR,1),"positive Pearson correlation");
        check(near(model.rmse,0),"linear exact rmse");
        check(std::isfinite(model.adjustedRSquared) && near(model.adjustedRSquared,1),
              "adjusted R squared");
        check(std::isfinite(model.diagonalRatio) && model.diagonalRatio>=1,
              "QR rank diagnostic");
        for (double xx : x) checkPred(model,xx,2*xx+1);
        checkPred(model,10,21);
    }
    {
        const std::vector<double> x{-3,-2,-1,0,1,2,3};
        std::vector<double> y;
        for(double v:x)y.push_back(1.-2*v+3*v*v);
        auto m=fitPolynomialRegression(x,y,2);
        check(m.successful(),"quadratic fit");
        check(!linearRegressionCoefficients(m).successful(),"linear summary rejects polynomial");
        check(near(m.rSquared,1),"quadratic r2");
        for(double v:x)checkPred(m,v,1.-2*v+3*v*v);
        checkPred(m,3.5,1-7+3*3.5*3.5,1e-8);
    }
    {
        const std::vector<double> x{-2,-1,0,1,2,3,4,5};
        std::vector<double> y;
        for(double v:x)y.push_back(3.+v-v*v+0.25*v*v*v);
        auto m=fitPolynomialRegression(x,y,3);
        check(m.successful(),"cubic fit");
        for(double v:x)checkPred(m,v,3.+v-v*v+.25*v*v*v);
    }
    {
        const std::vector<double> x{-2,-1,0,1,2,3,4,5};
        std::vector<double> y;
        for(double v:x)y.push_back(3.+v-v*v+0.25*v*v*v+0.125*v*v*v*v);
        auto m=fitPolynomialRegression(x,y,4);
        check(m.successful(),"quartic fit");
        for(double v:x)checkPred(m,v,3.+v-v*v+.25*v*v*v+.125*v*v*v*v,2e-8);
    }
    {
        const std::vector<double> x{-3,-2,-1,0,1,2,3};
        const std::vector<double> y{11,7,5,3,2,0,-1};
        auto m=fitLinearRegression(x,y);
        check(m.successful(),"noisy line fit");
        check(std::isfinite(m.sse) && m.sse>0,"noisy data positive SSE");
        check(std::isfinite(m.rmse) && m.rmse>0,"noisy data positive RMSE");
        check(m.rSquared>0 && m.rSquared<1,"noisy r2 bounded");
        check(m.pearsonR<0,"negative correlation");
        check(m.adjustedRSquared<m.rSquared,"adjusted r2 penalizes fit");
    }
    {
        const std::vector<double> x{0,1,2,3};
        const std::vector<double> y{7,7,7,7};
        auto m=fitLinearRegression(x,y);
        check(m.successful(),"constant dependent values accepted");
        check(std::isnan(m.pearsonR),"constant y Pearson undefined");
        check(std::isnan(m.adjustedRSquared),"constant y adjusted R2 undefined");
        check(near(m.rSquared,1),"perfect constant y R2 one");
        for(double v:x)checkPred(m,v,7);
    }
    {
        const std::vector<double> x{0,1};
        const std::vector<double> y{2,5};
        auto m=fitLinearRegression(x,y);
        check(m.successful(),"two point line accepted");
        check(std::isnan(m.adjustedRSquared),"two point adjusted r2 undefined");
        checkPred(m,2,8);
        check(fitPolynomialRegression(x,y,2).status==RegressionStatus::TooFewSamples,
              "degree 2 needs three observations");
    }
    {
        const std::vector<double> x{1,1,1,1};
        const std::vector<double> y{1,2,3,4};
        check(fitLinearRegression(x,y).status==RegressionStatus::RankDeficient,
              "constant predictor rank deficient");
    }
    {
        const std::vector<double> x{1,1,2,2,3,3};
        const std::vector<double> y{1,2,2,3,3,4};
        check(fitPolynomialRegression(x,y,3).status==RegressionStatus::RankDeficient,
              "only three distinct predictors cannot support cubic");
    }
    {
        std::vector<double> x{1e12,1e12+1,1e12+2,1e12+3,1e12+4,1e12+5};
        std::vector<double> y;
        for(double v:x)y.push_back(10.+.25*(v-1e12));
        auto m=fitLinearRegression(x,y);
        check(m.successful(),"large-offset x stable fit");
        for(double v:x)checkPred(m,v,10.+.25*(v-1e12));
        checkPred(m,1e12+6,11.5);
    }
    {
        const double inf=std::numeric_limits<double>::infinity();
        const double nan=std::numeric_limits<double>::quiet_NaN();
        std::vector<double> empty;
        std::vector<double> xx{0,1,2}, yy{0,1,2};
        check(fitLinearRegression(empty,empty).status==RegressionStatus::EmptyInput,"empty datasets");
        check(fitLinearRegression(xx,std::vector<double>{0,1}).status==RegressionStatus::SizeMismatch,"mismatched data");
        check(fitPolynomialRegression(xx,yy,0).status==RegressionStatus::InvalidDegree,"degree 0 rejected");
        check(fitPolynomialRegression(xx,yy,5).status==RegressionStatus::InvalidDegree,"degree >4 rejected");
        xx[1]=nan;
        check(fitLinearRegression(xx,yy).status==RegressionStatus::NonFiniteInput,"NaN predictor rejected");
        xx[1]=1; yy[1]=inf;
        check(fitLinearRegression(xx,yy).status==RegressionStatus::NonFiniteInput,"infinite outcome rejected");
        RegressionOptions bad;
        bad.maxSamples=1;
        check(fitLinearRegression(xx,xx,bad).status==RegressionStatus::InvalidDegree,"illegal sample option");
        bad=RegressionOptions{};bad.maxSamples=2;
        check(fitLinearRegression(xx,xx,bad).status==RegressionStatus::TooManySamples,"dataset cap");
        bad=RegressionOptions{};bad.maxDiagonalRatio=nan;
        check(fitLinearRegression(xx,xx,bad).status==RegressionStatus::InvalidDegree,"invalid conditioning option");
        bad=RegressionOptions{};bad.maxDiagonalRatio=0.5;
        check(fitLinearRegression(xx,xx,bad).status==RegressionStatus::InvalidDegree,"invalid threshold");
    }
    {
        std::vector<double> x{-2,-1,0,1,2}, y{-2,-1,0,1,2};
        auto model=fitLinearRegression(x,y);
        check(!predictRegression(model,std::numeric_limits<double>::quiet_NaN()).successful(),
              "nonfinite prediction rejected");
        check(!predictRegression(RegressionResult{},0).successful(),"uninitialized model rejected");
        check(std::string(regressionStatusName(RegressionStatus::IllConditioned))==
              "ill-conditioned design","readable status");
        check(predictRegression(model,1e308).status==RegressionStatus::Success,
              "wide but representable linear extrapolation");
    }
    {
        std::vector<double> x{-2,-1,0,1,2},y;
        for(double v:x)y.push_back(v*v*v*v);
        RegressionOptions o; o.maxDiagonalRatio=1;
        check(fitPolynomialRegression(x,y,4,o).status==RegressionStatus::IllConditioned,
              "strict conditioning gate rejects fit");
    }
    {
        std::vector<double> x{-1,0,1,2},y{-1,0,1,2};
        const auto m=fitLinearRegression(x,y);
        check(m.successful(),"coefficient-model semantics baseline");
        // Returned coefficients are in normalized t, not raw x.
        check(near(m.coefficients[0],m.xOffset),"normalized intercept semantics");
        check(near(m.coefficients[1],m.xScale),"normalized slope semantics");
    }
    {
        // Deterministic oracle cases, shuffled abscissas, noisy-free polynomials.
        std::mt19937_64 rng(0x52A7C33ULL);
        std::uniform_real_distribution<double> coefficient(-5.0,5.0);
        std::uniform_real_distribution<double> center(-1e5,1e5);
        for(int trial=0;trial<160;++trial) {
            const unsigned degree=1+static_cast<unsigned>(trial%4);
            std::vector<double> x,y;
            std::vector<double> c(degree+1);
            for(double &v:c)v=coefficient(rng);
            const double offset=center(rng);
            for(int i=0;i<12;++i) {
                const double t=-1.+2.*i/11.;
                const double value=offset+10*t;
                x.push_back(value);
                double yy=0;
                for(std::size_t j=c.size();j-- > 0;)yy=yy*t+c[j];
                y.push_back(yy);
            }
            std::vector<std::size_t> perm(x.size());
            for(std::size_t i=0;i<perm.size();++i)perm[i]=i;
            std::shuffle(perm.begin(),perm.end(),rng);
            std::vector<double> px,py;
            for(auto i:perm){px.push_back(x[i]);py.push_back(y[i]);}
            const auto model=fitPolynomialRegression(px,py,degree);
            check(model.successful(),"fixed-seed fit success");
            check(model.successful() && model.rSquared>0.99999999,"fixed-seed R2");
            if(model.successful()) {
                for(std::size_t i=0;i<x.size();++i)
                    checkPred(model,x[i],y[i],3e-8);
            }
        }
    }
    std::cout << (failures?"FAIL: ":"PASS: ") << assertions << " StatisticalRegression assertions\n";
    return failures?1:0;
}
