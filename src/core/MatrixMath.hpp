#pragma once

#include <cstddef>
#include <limits>
#include <vector>

namespace rationalstudio {

// M4-05 / Issue #50. Real-valued, bounded linear algebra, independent of Qt.
// These types are intentionally separate from the existing scalar Expression
// and complex Expression APIs. No implicit conversion between them is implied.
struct Matrix {
    std::size_t rows = 0;
    std::size_t columns = 0;
    std::vector<double> values; // row major
    [[nodiscard]] double at(std::size_t row, std::size_t column) const {
        return values.at(row * columns + column);
    }
};

struct MathVector {
    std::vector<double> values;
    [[nodiscard]] std::size_t size() const noexcept { return values.size(); }
};

enum class MatrixStatus {
    Success,
    InvalidShape,
    DimensionMismatch,
    NonFiniteInput,
    NonFiniteResult,
    Singular,
    IllConditioned,
    Unresolved
};

[[nodiscard]] const char* matrixStatusName(MatrixStatus status) noexcept;

struct MatrixResult {
    MatrixStatus status = MatrixStatus::InvalidShape;
    Matrix value;
    [[nodiscard]] bool successful() const noexcept { return status == MatrixStatus::Success; }
};
struct VectorResult {
    MatrixStatus status = MatrixStatus::InvalidShape;
    MathVector value;
    [[nodiscard]] bool successful() const noexcept { return status == MatrixStatus::Success; }
};
struct ScalarMatrixResult {
    MatrixStatus status = MatrixStatus::InvalidShape;
    double value = std::numeric_limits<double>::quiet_NaN();
    [[nodiscard]] bool successful() const noexcept { return status == MatrixStatus::Success; }
};

// Maximum dimension is 128, total entries <= 16,384. Linear solves and
// determinants are limited further to square systems of dimension <= 64.
[[nodiscard]] MatrixResult makeMatrix(std::size_t rows, std::size_t columns,
                                      const std::vector<double>& rowMajor);
[[nodiscard]] VectorResult makeVector(const std::vector<double>& entries);
[[nodiscard]] MatrixResult addMatrices(const Matrix& lhs, const Matrix& rhs);
[[nodiscard]] MatrixResult subtractMatrices(const Matrix& lhs, const Matrix& rhs);
[[nodiscard]] MatrixResult scaleMatrix(const Matrix& matrix, double factor);
[[nodiscard]] MatrixResult multiplyMatrices(const Matrix& lhs, const Matrix& rhs);
[[nodiscard]] MatrixResult transposeMatrix(const Matrix& matrix);
[[nodiscard]] VectorResult multiplyMatrixVector(const Matrix& matrix,
                                                const MathVector& vector);
[[nodiscard]] VectorResult addVectors(const MathVector& lhs, const MathVector& rhs);
[[nodiscard]] VectorResult subtractVectors(const MathVector& lhs, const MathVector& rhs);
[[nodiscard]] VectorResult scaleVector(const MathVector& vector, double factor);
[[nodiscard]] ScalarMatrixResult dotVectors(const MathVector& lhs, const MathVector& rhs);
[[nodiscard]] ScalarMatrixResult normVector(const MathVector& vector);
[[nodiscard]] ScalarMatrixResult determinant(const Matrix& square);

struct LinearSolveOptions {
    // Reject systems with ||A||_inf * ||A^-1||_inf > 1/minReciprocalCondition.
    double minReciprocalCondition = 1e-12;
    // Require ||Ax-b||_inf/(||A||_inf ||x||_inf+||b||_inf) <= threshold.
    double relativeResidualTolerance = 1e-10;
    std::size_t maxRefinements = 2;  // 0..8
};
struct LinearSolveResult {
    MatrixStatus status = MatrixStatus::InvalidShape;
    MathVector solution; // published only on Success
    double residualInfinityNorm = std::numeric_limits<double>::infinity();
    double relativeResidual = std::numeric_limits<double>::infinity();
    double reciprocalCondition = 0.0;
    std::size_t refinements = 0;
    [[nodiscard]] bool successful() const noexcept { return status == MatrixStatus::Success; }
};
// LU with partial pivoting, ||.||inf-based reciprocal-condition calculation,
// and residual verification. Not a certified forward-error bound.
[[nodiscard]] LinearSolveResult solveLinearSystem(
    const Matrix& coefficients, const MathVector& rhs,
    const LinearSolveOptions& options = {});

} // namespace rationalstudio
