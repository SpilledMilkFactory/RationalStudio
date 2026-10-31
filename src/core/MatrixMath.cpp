#include "MatrixMath.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <utility>

namespace rationalstudio {
namespace {
constexpr std::size_t kMaxDimension = 128;
constexpr std::size_t kMaxEntries = 16384;
constexpr std::size_t kMaxFactorDimension = 64;

bool finite(long double value) { return std::isfinite(value); }
bool toDouble(long double input, double& output) {
    if (!finite(input) || std::abs(input) > std::numeric_limits<double>::max()) return false;
    output = static_cast<double>(input);
    return std::isfinite(output);
}
MatrixStatus validate(const Matrix& m) {
    if (!m.rows || !m.columns || m.rows > kMaxDimension || m.columns > kMaxDimension ||
        m.rows * m.columns > kMaxEntries || m.values.size() != m.rows * m.columns)
        return MatrixStatus::InvalidShape;
    for (double value : m.values) if (!std::isfinite(value)) return MatrixStatus::NonFiniteInput;
    return MatrixStatus::Success;
}
MatrixStatus validate(const MathVector& v) {
    if (v.values.empty() || v.values.size() > kMaxEntries) return MatrixStatus::InvalidShape;
    for (double value : v.values) if (!std::isfinite(value)) return MatrixStatus::NonFiniteInput;
    return MatrixStatus::Success;
}
MatrixResult errorM(MatrixStatus status) { return {status, {}}; }
VectorResult errorV(MatrixStatus status) { return {status, {}}; }
ScalarMatrixResult errorS(MatrixStatus status) { return {status, std::numeric_limits<double>::quiet_NaN()}; }

struct Factorization {
    std::size_t n = 0;
    std::vector<long double> lu;
    std::vector<std::size_t> swaps;
    int sign = 1;
    bool success = false;
};
Factorization factor(const Matrix& m) {
    Factorization f;
    f.n = m.rows;
    f.lu.assign(m.values.begin(), m.values.end());
    f.swaps.resize(f.n);
    for (std::size_t k = 0; k < f.n; ++k) {
        std::size_t pivot = k;
        for (std::size_t row = k + 1; row < f.n; ++row)
            if (std::abs(f.lu[row * f.n + k]) > std::abs(f.lu[pivot * f.n + k])) pivot = row;
        if (f.lu[pivot * f.n + k] == 0 || !finite(f.lu[pivot * f.n + k])) return f;
        f.swaps[k] = pivot;
        if (pivot != k) {
            for (std::size_t col = 0; col < f.n; ++col)
                std::swap(f.lu[k * f.n + col], f.lu[pivot * f.n + col]);
            f.sign = -f.sign;
        }
        const long double diagonal = f.lu[k * f.n + k];
        for (std::size_t row = k + 1; row < f.n; ++row) {
            f.lu[row * f.n + k] /= diagonal;
            for (std::size_t col = k + 1; col < f.n; ++col)
                f.lu[row * f.n + col] -= f.lu[row * f.n + k] * f.lu[k * f.n + col];
        }
    }
    f.success = std::all_of(f.lu.begin(), f.lu.end(), [](long double x) { return finite(x); });
    return f;
}
bool substitute(const Factorization& f, const std::vector<long double>& rhs,
                std::vector<long double>& x) {
    if (!f.success || rhs.size() != f.n) return false;
    x = rhs;
    for (std::size_t k = 0; k < f.n; ++k) std::swap(x[k], x[f.swaps[k]]);
    for (std::size_t row = 0; row < f.n; ++row)
        for (std::size_t col = 0; col < row; ++col)
            x[row] -= f.lu[row * f.n + col] * x[col];
    for (std::size_t row = f.n; row-- > 0;) {
        for (std::size_t col = row + 1; col < f.n; ++col)
            x[row] -= f.lu[row * f.n + col] * x[col];
        x[row] /= f.lu[row * f.n + row];
    }
    return std::all_of(x.begin(), x.end(), [](long double x0) { return finite(x0); });
}
long double infNormMatrix(const Matrix& m) {
    long double result = 0;
    for (std::size_t row = 0; row < m.rows; ++row) {
        long double sum = 0;
        for (std::size_t col = 0; col < m.columns; ++col)
            sum += std::abs(static_cast<long double>(m.at(row, col)));
        result = std::max(result, sum);
    }
    return result;
}
long double infNorm(const std::vector<long double>& v) {
    long double result = 0;
    for (auto value : v) result = std::max(result, std::abs(value));
    return result;
}
long double backwardError(const Matrix& m, const std::vector<long double>& x,
                         const std::vector<long double>& b, std::vector<long double>& residual,
                         long double normA) {
    residual.assign(m.rows, 0);
    for (std::size_t i = 0; i < m.rows; ++i) {
        long double ax = 0;
        for (std::size_t j = 0; j < m.columns; ++j)
            ax += static_cast<long double>(m.at(i,j)) * x[j];
        residual[i] = b[i] - ax;
    }
    const long double denominator = normA * infNorm(x) + infNorm(b);
    return denominator > 0 ? infNorm(residual) / denominator : infNorm(residual);
}
}

const char* matrixStatusName(MatrixStatus status) noexcept {
    switch (status) {
    case MatrixStatus::Success: return "success";
    case MatrixStatus::InvalidShape: return "invalid shape";
    case MatrixStatus::DimensionMismatch: return "dimension mismatch";
    case MatrixStatus::NonFiniteInput: return "nonfinite input";
    case MatrixStatus::NonFiniteResult: return "nonfinite result";
    case MatrixStatus::Singular: return "singular matrix";
    case MatrixStatus::IllConditioned: return "ill-conditioned matrix";
    case MatrixStatus::Unresolved: return "unresolved numerical residual";
    }
    return "unknown";
}
MatrixResult makeMatrix(std::size_t rows, std::size_t columns,
                        const std::vector<double>& rowMajor) {
    Matrix m{rows, columns, rowMajor};
    const auto status = validate(m);
    return status == MatrixStatus::Success ? MatrixResult{status, std::move(m)} : errorM(status);
}
VectorResult makeVector(const std::vector<double>& entries) {
    MathVector v{entries};
    const auto status = validate(v);
    return status == MatrixStatus::Success ? VectorResult{status, std::move(v)} : errorV(status);
}
MatrixResult addMatrices(const Matrix& lhs, const Matrix& rhs) {
    if (auto s = validate(lhs); s != MatrixStatus::Success) return errorM(s);
    if (auto s = validate(rhs); s != MatrixStatus::Success) return errorM(s);
    if (lhs.rows != rhs.rows || lhs.columns != rhs.columns) return errorM(MatrixStatus::DimensionMismatch);
    Matrix out = lhs;
    for (std::size_t i = 0; i < out.values.size(); ++i)
        if (!toDouble(static_cast<long double>(lhs.values[i]) + rhs.values[i], out.values[i]))
            return errorM(MatrixStatus::NonFiniteResult);
    return {MatrixStatus::Success, std::move(out)};
}
MatrixResult subtractMatrices(const Matrix& lhs, const Matrix& rhs) {
    if (auto s = validate(lhs); s != MatrixStatus::Success) return errorM(s);
    if (auto s = validate(rhs); s != MatrixStatus::Success) return errorM(s);
    if (lhs.rows != rhs.rows || lhs.columns != rhs.columns) return errorM(MatrixStatus::DimensionMismatch);
    Matrix out = lhs;
    for (std::size_t i = 0; i < out.values.size(); ++i)
        if (!toDouble(static_cast<long double>(lhs.values[i]) - rhs.values[i], out.values[i]))
            return errorM(MatrixStatus::NonFiniteResult);
    return {MatrixStatus::Success, std::move(out)};
}
MatrixResult scaleMatrix(const Matrix& m, double factor) {
    if (auto s = validate(m); s != MatrixStatus::Success) return errorM(s);
    if (!std::isfinite(factor)) return errorM(MatrixStatus::NonFiniteInput);
    Matrix out = m;
    for (double& value : out.values)
        if (!toDouble(static_cast<long double>(value) * factor, value))
            return errorM(MatrixStatus::NonFiniteResult);
    return {MatrixStatus::Success, std::move(out)};
}
MatrixResult multiplyMatrices(const Matrix& lhs, const Matrix& rhs) {
    if (auto s = validate(lhs); s != MatrixStatus::Success) return errorM(s);
    if (auto s = validate(rhs); s != MatrixStatus::Success) return errorM(s);
    if (lhs.columns != rhs.rows) return errorM(MatrixStatus::DimensionMismatch);
    Matrix out{lhs.rows, rhs.columns, std::vector<double>(lhs.rows * rhs.columns)};
    for (std::size_t i = 0; i < out.rows; ++i)
        for (std::size_t j = 0; j < out.columns; ++j) {
            long double sum = 0;
            for (std::size_t k = 0; k < lhs.columns; ++k)
                sum += static_cast<long double>(lhs.at(i,k)) * rhs.at(k,j);
            if (!toDouble(sum, out.values[i*out.columns+j])) return errorM(MatrixStatus::NonFiniteResult);
        }
    return {MatrixStatus::Success, std::move(out)};
}
MatrixResult transposeMatrix(const Matrix& m) {
    if (auto s = validate(m); s != MatrixStatus::Success) return errorM(s);
    Matrix out{m.columns, m.rows, std::vector<double>(m.values.size())};
    for (std::size_t i = 0; i < m.rows; ++i)
        for (std::size_t j = 0; j < m.columns; ++j) out.values[j*out.columns+i] = m.at(i,j);
    return {MatrixStatus::Success, std::move(out)};
}
VectorResult multiplyMatrixVector(const Matrix& m, const MathVector& v) {
    if (auto s = validate(m); s != MatrixStatus::Success) return errorV(s);
    if (auto s = validate(v); s != MatrixStatus::Success) return errorV(s);
    if (m.columns != v.size()) return errorV(MatrixStatus::DimensionMismatch);
    MathVector out{std::vector<double>(m.rows)};
    for (std::size_t i = 0; i < m.rows; ++i) {
        long double sum = 0;
        for (std::size_t j = 0; j < m.columns; ++j)
            sum += static_cast<long double>(m.at(i,j)) * v.values[j];
        if (!toDouble(sum, out.values[i])) return errorV(MatrixStatus::NonFiniteResult);
    }
    return {MatrixStatus::Success, std::move(out)};
}
VectorResult addVectors(const MathVector& lhs, const MathVector& rhs) {
    if (auto s = validate(lhs); s != MatrixStatus::Success) return errorV(s);
    if (auto s = validate(rhs); s != MatrixStatus::Success) return errorV(s);
    if (lhs.size() != rhs.size()) return errorV(MatrixStatus::DimensionMismatch);
    MathVector out = lhs;
    for (std::size_t i=0;i<lhs.size();++i)
        if (!toDouble(static_cast<long double>(lhs.values[i])+rhs.values[i],out.values[i]))
            return errorV(MatrixStatus::NonFiniteResult);
    return {MatrixStatus::Success,std::move(out)};
}
VectorResult subtractVectors(const MathVector& lhs, const MathVector& rhs) {
    if (auto s = validate(lhs); s != MatrixStatus::Success) return errorV(s);
    if (auto s = validate(rhs); s != MatrixStatus::Success) return errorV(s);
    if (lhs.size() != rhs.size()) return errorV(MatrixStatus::DimensionMismatch);
    MathVector out = lhs;
    for (std::size_t i=0;i<lhs.size();++i)
        if (!toDouble(static_cast<long double>(lhs.values[i])-rhs.values[i],out.values[i]))
            return errorV(MatrixStatus::NonFiniteResult);
    return {MatrixStatus::Success,std::move(out)};
}
VectorResult scaleVector(const MathVector& v, double factor) {
    if (auto s=validate(v);s!=MatrixStatus::Success) return errorV(s);
    if (!std::isfinite(factor)) return errorV(MatrixStatus::NonFiniteInput);
    MathVector out = v;
    for (double& value:out.values)
        if (!toDouble(static_cast<long double>(value)*factor,value))
            return errorV(MatrixStatus::NonFiniteResult);
    return {MatrixStatus::Success,std::move(out)};
}
ScalarMatrixResult dotVectors(const MathVector& lhs, const MathVector& rhs) {
    if (auto s=validate(lhs);s!=MatrixStatus::Success) return errorS(s);
    if (auto s=validate(rhs);s!=MatrixStatus::Success) return errorS(s);
    if (lhs.size()!=rhs.size()) return errorS(MatrixStatus::DimensionMismatch);
    long double sum=0;
    for (std::size_t i=0;i<lhs.size();++i)
        sum+=static_cast<long double>(lhs.values[i])*rhs.values[i];
    double out;
    return toDouble(sum,out) ? ScalarMatrixResult{MatrixStatus::Success,out} : errorS(MatrixStatus::NonFiniteResult);
}
ScalarMatrixResult normVector(const MathVector& v) {
    if (auto s=validate(v);s!=MatrixStatus::Success) return errorS(s);
    long double sum=0;
    for (double x:v.values) sum=std::hypot(sum,static_cast<long double>(x));
    double out;
    return toDouble(sum,out) ? ScalarMatrixResult{MatrixStatus::Success,out} : errorS(MatrixStatus::NonFiniteResult);
}
ScalarMatrixResult determinant(const Matrix& m) {
    if (auto s=validate(m);s!=MatrixStatus::Success) return errorS(s);
    if (m.rows!=m.columns || m.rows>kMaxFactorDimension) return errorS(MatrixStatus::InvalidShape);
    const auto f=factor(m);
    if (!f.success) return {MatrixStatus::Success,0.0}; // exact singular determinant
    long double d=f.sign;
    for (std::size_t i=0;i<f.n;++i) d*=f.lu[i*f.n+i];
    double out;
    return toDouble(d,out) ? ScalarMatrixResult{MatrixStatus::Success,out} : errorS(MatrixStatus::NonFiniteResult);
}
LinearSolveResult solveLinearSystem(const Matrix& m, const MathVector& rhs,
                                    const LinearSolveOptions& options) {
    LinearSolveResult out;
    if (auto s=validate(m);s!=MatrixStatus::Success) {out.status=s;return out;}
    if (auto s=validate(rhs);s!=MatrixStatus::Success) {out.status=s;return out;}
    if (m.rows!=m.columns || m.rows>kMaxFactorDimension) {out.status=MatrixStatus::InvalidShape;return out;}
    if (rhs.size()!=m.rows) {out.status=MatrixStatus::DimensionMismatch;return out;}
    if (!std::isfinite(options.minReciprocalCondition) || options.minReciprocalCondition<=0 ||
        options.minReciprocalCondition>=1 || !std::isfinite(options.relativeResidualTolerance) ||
        options.relativeResidualTolerance<=0 || options.relativeResidualTolerance>=1 ||
        options.maxRefinements>8) {out.status=MatrixStatus::InvalidShape;return out;}
    const auto f=factor(m);
    if (!f.success) {out.status=MatrixStatus::Singular;return out;}
    const long double normA=infNormMatrix(m);
    std::vector<long double> rowSums(m.rows,0);
    std::vector<long double> basis(m.rows,0),column;
    for (std::size_t j=0;j<m.rows;++j) {
        basis[j]=1;
        if (!substitute(f,basis,column)) {out.status=MatrixStatus::Unresolved;return out;}
        basis[j]=0;
        for (std::size_t i=0;i<m.rows;++i) rowSums[i]+=std::abs(column[i]);
    }
    const long double normInverse=*std::max_element(rowSums.begin(),rowSums.end());
    const long double condition=normA*normInverse;
    if (!finite(condition) || condition<=0) {out.status=MatrixStatus::Unresolved;return out;}
    const long double rcond=1.0L/condition;
    out.reciprocalCondition=static_cast<double>(rcond);
    if (rcond < options.minReciprocalCondition) {out.status=MatrixStatus::IllConditioned;return out;}
    const std::vector<long double> b(rhs.values.begin(),rhs.values.end());
    std::vector<long double> x,residual;
    if (!substitute(f,b,x)) {out.status=MatrixStatus::Unresolved;return out;}
    for (std::size_t trial=0;;++trial) {
        const long double backward=backwardError(m,x,b,residual,normA);
        if (!finite(backward)) {out.status=MatrixStatus::Unresolved;return out;}
        if (backward <= options.relativeResidualTolerance) {
            out.solution.values.resize(x.size());
            for (std::size_t i=0;i<x.size();++i)
                if (!toDouble(x[i],out.solution.values[i])) {
                    out.solution.values.clear();out.status=MatrixStatus::NonFiniteResult;return out;
                }
            // Check the residual of the actually *published* double solution.
            std::vector<long double> rounded(out.solution.values.begin(),out.solution.values.end());
            const long double verified=backwardError(m,rounded,b,residual,normA);
            if (verified>options.relativeResidualTolerance || !finite(verified)) {
                out.solution.values.clear();out.status=MatrixStatus::Unresolved;return out;
            }
            double absolute;
            if (!toDouble(infNorm(residual),absolute)) {out.solution.values.clear();out.status=MatrixStatus::NonFiniteResult;return out;}
            out.residualInfinityNorm=absolute;
            out.relativeResidual=static_cast<double>(verified);
            out.status=MatrixStatus::Success;
            return out;
        }
        if (trial>=options.maxRefinements) {out.status=MatrixStatus::Unresolved;return out;}
        std::vector<long double> correction;
        if (!substitute(f,residual,correction)) {out.status=MatrixStatus::Unresolved;return out;}
        for (std::size_t i=0;i<x.size();++i) x[i]+=correction[i];
        ++out.refinements;
    }
}
} // namespace rationalstudio
