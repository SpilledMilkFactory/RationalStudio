#include "StatisticalRegression.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace rationalstudio {
namespace {
constexpr double kDoubleMax = std::numeric_limits<double>::max();

bool publish(long double input, double& out) noexcept {
    if (!std::isfinite(input) || std::abs(input) > static_cast<long double>(kDoubleMax))
        return false;
    out = static_cast<double>(input);
    return std::isfinite(out);
}

long double evaluate(const std::vector<double>& a, long double t) noexcept {
    long double result = 0;
    for (std::size_t i = a.size(); i-- > 0;)
        result = result * t + static_cast<long double>(a[i]);
    return result;
}
} // namespace

const char* regressionStatusName(RegressionStatus status) noexcept {
    switch (status) {
    case RegressionStatus::Success: return "success";
    case RegressionStatus::EmptyInput: return "empty input";
    case RegressionStatus::SizeMismatch: return "size mismatch";
    case RegressionStatus::TooFewSamples: return "too few samples";
    case RegressionStatus::TooManySamples: return "sample limit exceeded";
    case RegressionStatus::NonFiniteInput: return "non-finite input";
    case RegressionStatus::InvalidDegree: return "invalid degree or options";
    case RegressionStatus::RankDeficient: return "rank deficient";
    case RegressionStatus::IllConditioned: return "ill-conditioned design";
    case RegressionStatus::NonFiniteResult: return "non-finite result";
    case RegressionStatus::InvalidPrediction: return "invalid prediction";
    }
    return "unknown";
}

LinearRegressionCoefficients linearRegressionCoefficients(
    const RegressionResult& model) noexcept {
    LinearRegressionCoefficients result;
    if (!model.successful() || model.degree != 1 || model.coefficients.size() != 2 ||
        !(model.xScale > 0) || !std::isfinite(model.xScale) ||
        !std::isfinite(model.xOffset)) return result;
    const long double slope = static_cast<long double>(model.coefficients[1]) /
        static_cast<long double>(model.xScale);
    const long double intercept = static_cast<long double>(model.coefficients[0]) -
        slope * static_cast<long double>(model.xOffset);
    if (!publish(slope, result.slope) || !publish(intercept, result.intercept)) {
        result.status = RegressionStatus::NonFiniteResult;
        return result;
    }
    result.status = RegressionStatus::Success;
    return result;
}

RegressionPrediction predictRegression(const RegressionResult& model, double x) noexcept {
    RegressionPrediction result;
    if (!model.successful() || !std::isfinite(x) ||
        !std::isfinite(model.xOffset) || !std::isfinite(model.xScale) ||
        !(model.xScale > 0) || model.coefficients.size() != model.degree + 1 ||
        model.coefficients.empty()) return result;
    const long double t = (static_cast<long double>(x) - model.xOffset) /
        static_cast<long double>(model.xScale);
    if (publish(evaluate(model.coefficients, t), result.value))
        result.status = RegressionStatus::Success;
    else result.status = RegressionStatus::NonFiniteResult;
    return result;
}

RegressionResult fitLinearRegression(std::span<const double> x,
                                     std::span<const double> y,
                                     const RegressionOptions& options) {
    return fitPolynomialRegression(x, y, 1, options);
}

RegressionResult fitPolynomialRegression(std::span<const double> x,
                                         std::span<const double> y,
                                         unsigned degree,
                                         const RegressionOptions& options) {
    RegressionResult out;
    out.degree = degree;
    if (degree < 1 || degree > 4 || options.maxSamples < 2 ||
        options.maxSamples > 100'000 || !std::isfinite(options.maxDiagonalRatio) ||
        options.maxDiagonalRatio < 1.0) {
        out.status = RegressionStatus::InvalidDegree;
        return out;
    }
    if (x.empty() || y.empty()) return out;
    if (x.size() != y.size()) { out.status = RegressionStatus::SizeMismatch; return out; }
    const std::size_t n = x.size(), p = degree + 1;
    if (n < p) { out.status = RegressionStatus::TooFewSamples; return out; }
    if (n > options.maxSamples) { out.status = RegressionStatus::TooManySamples; return out; }
    long double minX = x.front(), maxX = x.front(), sumY = 0;
    for (std::size_t i = 0; i < n; ++i) {
        if (!std::isfinite(x[i]) || !std::isfinite(y[i])) {
            out.status = RegressionStatus::NonFiniteInput; return out;
        }
        minX = std::min(minX, static_cast<long double>(x[i]));
        maxX = std::max(maxX, static_cast<long double>(x[i]));
        sumY += static_cast<long double>(y[i]);
    }
    // Normalize x and y before QR; the final model is still in original y units.
    // Store a representable double x center to make prediction repeatable.
    double offset = 0, scale = 0, yCenter = 0, yScale = 0;
    if (!publish(minX + (maxX - minX) * 0.5L, offset) ||
        !publish(sumY / static_cast<long double>(n), yCenter)) {
        out.status = RegressionStatus::NonFiniteResult; return out;
    }
    long double maxDx = 0, maxDy = 0, sumY2 = 0;
    for (std::size_t i = 0; i < n; ++i) {
        maxDx = std::max(maxDx, std::abs(static_cast<long double>(x[i]) - offset));
        maxDy = std::max(maxDy, std::abs(static_cast<long double>(y[i]) - yCenter));
        const long double dy = static_cast<long double>(y[i]) - yCenter;
        sumY2 += dy * dy;
    }
    if (maxDx == 0) { out.status = RegressionStatus::RankDeficient; return out; }
    if (!publish(maxDx, scale) || !publish(maxDy > 0 ? maxDy : 1.0L, yScale) ||
        !(scale > 0) || !(yScale > 0)) {
        out.status = RegressionStatus::NonFiniteResult; return out;
    }
    // Row-major n x p matrix, normalized monomial basis.
    std::vector<long double> matrix(n * p), rhs(n);
    for (std::size_t i = 0; i < n; ++i) {
        const long double t = (static_cast<long double>(x[i]) - offset) / scale;
        rhs[i] = (static_cast<long double>(y[i]) - yCenter) / yScale;
        long double power = 1;
        for (std::size_t j = 0; j < p; ++j) {
            matrix[i*p+j] = power;
            power *= t;
        }
    }
    std::vector<std::size_t> perm(p);
    std::iota(perm.begin(), perm.end(), 0);
    long double maxDiagonal = 0, minDiagonal = std::numeric_limits<long double>::infinity();
    // Column-pivoted Householder QR; first rotate the design, then RHS.
    for (std::size_t k = 0; k < p; ++k) {
        std::size_t pivot = k;
        long double largestNorm = -1;
        for (std::size_t j = k; j < p; ++j) {
            long double length2 = 0;
            for (std::size_t i = k; i < n; ++i) length2 += matrix[i*p+j] * matrix[i*p+j];
            if (length2 > largestNorm) { largestNorm = length2; pivot = j; }
        }
        if (!(largestNorm > 0) || !std::isfinite(largestNorm)) {
            out.status = RegressionStatus::RankDeficient; return out;
        }
        if (pivot != k) {
            for (std::size_t i = 0; i < n; ++i)
                std::swap(matrix[i*p+k], matrix[i*p+pivot]);
            std::swap(perm[k], perm[pivot]);
        }
        const long double norm = std::sqrt(largestNorm);
        if (maxDiagonal > 0 && norm <= maxDiagonal *
            (128.0L * std::numeric_limits<long double>::epsilon() * p)) {
            out.status = RegressionStatus::RankDeficient; return out;
        }
        const long double original = matrix[k*p+k];
        const long double alpha = original >= 0 ? -norm : norm;
        const long double head = original - alpha;
        long double vSquared = head * head;
        for (std::size_t i = k+1; i < n; ++i)
            vSquared += matrix[i*p+k] * matrix[i*p+k];
        if (!(vSquared > 0) || !std::isfinite(vSquared)) {
            out.status = RegressionStatus::RankDeficient; return out;
        }
        const long double beta = 2.0L / vSquared;
        // Apply reflection to all remaining columns, but overwrite the
        // reflector's original column with its triangular R entries afterward.
        for (std::size_t j = k+1; j < p; ++j) {
            long double dot = head * matrix[k*p+j];
            for (std::size_t i = k+1; i < n; ++i) dot += matrix[i*p+k]*matrix[i*p+j];
            dot *= beta;
            matrix[k*p+j] -= dot * head;
            for (std::size_t i = k+1; i < n; ++i) matrix[i*p+j] -= dot*matrix[i*p+k];
        }
        long double dot = head * rhs[k];
        for (std::size_t i = k+1; i < n; ++i) dot += matrix[i*p+k] * rhs[i];
        dot *= beta;
        rhs[k] -= dot * head;
        for (std::size_t i = k+1; i < n; ++i) rhs[i] -= dot*matrix[i*p+k];
        matrix[k*p+k] = alpha;
        for (std::size_t i = k+1; i < n; ++i) matrix[i*p+k] = 0;
        minDiagonal = std::min(minDiagonal, norm);
        maxDiagonal = std::max(maxDiagonal, norm);
    }
    const long double ratio = maxDiagonal / minDiagonal;
    if (!std::isfinite(ratio) || !publish(ratio, out.diagonalRatio)) {
        out.status = RegressionStatus::NonFiniteResult; return out;
    }
    if (ratio > options.maxDiagonalRatio) {
        out.status = RegressionStatus::IllConditioned; return out;
    }
    // Solve triangular system and undo column pivoting.
    std::vector<long double> solution(p), unpermuted(p);
    for (std::size_t k = p; k-- > 0;) {
        long double term = rhs[k];
        for (std::size_t j = k+1; j < p; ++j) term -= matrix[k*p+j]*solution[j];
        solution[k] = term / matrix[k*p+k];
        if (!std::isfinite(solution[k])) {
            out.status = RegressionStatus::NonFiniteResult; return out;
        }
    }
    for (std::size_t j = 0; j < p; ++j) unpermuted[perm[j]] = solution[j];
    std::vector<double> coefficients(p);
    for (std::size_t k = 0; k < p; ++k) {
        const long double value = (k == 0 ? static_cast<long double>(yCenter) : 0.0L)
            + static_cast<long double>(yScale) * unpermuted[k];
        if (!publish(value, coefficients[k])) {
            out.status = RegressionStatus::NonFiniteResult; return out;
        }
    }
    // Calculate diagnostics from the published coefficients, not the internal
    // extra-precision estimates, so diagnostics reflect actual API predictions.
    long double sse = 0, sumYY = 0, sumXX = 0, sumXY = 0;
    const long double meanX = std::accumulate(x.begin(), x.end(), 0.0L) / n;
    for (std::size_t i = 0; i < n; ++i) {
        const long double t = (static_cast<long double>(x[i]) - offset) / scale;
        const long double residual = static_cast<long double>(y[i]) - evaluate(coefficients,t);
        sse += residual * residual;
        const long double yc = static_cast<long double>(y[i]) - yCenter;
        const long double xc = static_cast<long double>(x[i]) - meanX;
        sumYY += yc * yc;
        if (degree == 1) { sumXX += xc * xc; sumXY += xc * yc; }
    }
    double sseOut = 0, rmse = 0, r2 = std::numeric_limits<double>::quiet_NaN();
    if (!publish(sse, sseOut) || !publish(std::sqrt(sse/n), rmse)) {
        out.status = RegressionStatus::NonFiniteResult; return out;
    }
    if (sumYY > 0) {
        if (!publish(1.0L - sse / sumYY, r2)) {
            out.status = RegressionStatus::NonFiniteResult; return out;
        }
    } else if (sse == 0) r2 = 1;
    double adjusted = std::numeric_limits<double>::quiet_NaN();
    if (std::isfinite(r2) && n > p && sumYY > 0 &&
        !publish(1.0L - (1.0L - r2) * (n-1.0L) / (n-p), adjusted)) {
        out.status = RegressionStatus::NonFiniteResult; return out;
    }
    double correlation = std::numeric_limits<double>::quiet_NaN();
    if (degree == 1 && sumYY > 0 && sumXX > 0 &&
        !publish(sumXY / std::sqrt(sumXX*sumYY), correlation)) {
        out.status = RegressionStatus::NonFiniteResult; return out;
    }
    out.status = RegressionStatus::Success;
    out.observations = n;
    out.coefficients = std::move(coefficients);
    out.xOffset = offset;
    out.xScale = scale;
    out.sse = sseOut;
    out.rmse = rmse;
    out.rSquared = r2;
    out.adjustedRSquared = adjusted;
    out.pearsonR = correlation;
    return out;
}
} // namespace rationalstudio
