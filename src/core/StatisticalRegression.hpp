#pragma once

#include <cstddef>
#include <limits>
#include <span>
#include <vector>

namespace rationalstudio {

// M4-07 / Issue #52: bounded real least-squares regression.
// Coefficients use t=(x-xOffset)/xScale to avoid unstable monomial powers
// on large-offset domains. They are NOT coefficients in raw powers of x.
enum class RegressionStatus {
    Success, EmptyInput, SizeMismatch, TooFewSamples, TooManySamples,
    NonFiniteInput, InvalidDegree, RankDeficient, IllConditioned,
    NonFiniteResult, InvalidPrediction
};

[[nodiscard]] const char* regressionStatusName(RegressionStatus status) noexcept;

struct RegressionOptions {
    // Bounded QR workspace: at most 100,000 observations, degree <= 4.
    std::size_t maxSamples = 100'000;
    // A heuristic R-diagonal-ratio threshold, NOT a certified condition number.
    double maxDiagonalRatio = 1e10;
};

struct RegressionResult {
    RegressionStatus status = RegressionStatus::EmptyInput;
    std::size_t observations = 0;
    unsigned degree = 0;
    // p(t)=sum(coefficients[k]*t^k), t=(x-xOffset)/xScale.
    std::vector<double> coefficients;
    double xOffset = std::numeric_limits<double>::quiet_NaN();
    double xScale = std::numeric_limits<double>::quiet_NaN();
    double sse = std::numeric_limits<double>::quiet_NaN();
    double rmse = std::numeric_limits<double>::quiet_NaN();
    double rSquared = std::numeric_limits<double>::quiet_NaN();
    // Undefined when observations <= number of parameters or y is constant.
    double adjustedRSquared = std::numeric_limits<double>::quiet_NaN();
    // For degree 1, only defined when y is nonconstant.
    double pearsonR = std::numeric_limits<double>::quiet_NaN();
    // Diagnostic only: ratio of max/min absolute R diagonals after pivoted QR.
    double diagonalRatio = std::numeric_limits<double>::quiet_NaN();
    [[nodiscard]] bool successful() const noexcept {
        return status == RegressionStatus::Success;
    }
};

// Optional conventional y = intercept + slope*x representation. For extreme
// offsets this expansion can overflow or lose relative accuracy; prefer
// predictRegression() and centered/scaled coefficients for evaluation.
struct LinearRegressionCoefficients {
    RegressionStatus status = RegressionStatus::InvalidPrediction;
    double intercept = std::numeric_limits<double>::quiet_NaN();
    double slope = std::numeric_limits<double>::quiet_NaN();
    [[nodiscard]] bool successful() const noexcept {
        return status == RegressionStatus::Success;
    }
};

[[nodiscard]] LinearRegressionCoefficients linearRegressionCoefficients(
    const RegressionResult& model) noexcept;

struct RegressionPrediction {
    RegressionStatus status = RegressionStatus::InvalidPrediction;
    double value = std::numeric_limits<double>::quiet_NaN();
    [[nodiscard]] bool successful() const noexcept {
        return status == RegressionStatus::Success;
    }
};

// Householder QR with column pivoting, without forming normal equations.
// Includes an intercept; valid degrees 1..4, n >= degree+1.
// Rejects rank deficiency, excessive diagonal ratio, nonfinite metrics.
[[nodiscard]] RegressionResult fitPolynomialRegression(
    std::span<const double> x, std::span<const double> y,
    unsigned degree, const RegressionOptions& options = {});

[[nodiscard]] RegressionResult fitLinearRegression(
    std::span<const double> x, std::span<const double> y,
    const RegressionOptions& options = {});

[[nodiscard]] RegressionPrediction predictRegression(
    const RegressionResult& model, double x) noexcept;

} // namespace rationalstudio
