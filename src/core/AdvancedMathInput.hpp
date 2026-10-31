#pragma once

#include "ComplexMath.hpp"
#include "MatrixMath.hpp"
#include "DescriptiveStatistics.hpp"
#include "StatisticalRegression.hpp"

#include <string>
#include <variant>
#include <vector>

namespace rationalstudio {

// Issue #53: presentation-layer transport, not a replacement Expression AST.
// The active alternative retains the mathematical type across the UI boundary.
using AdvancedMathValue = std::variant<double, ComplexValue, Matrix, MathVector,
                                       DescriptiveStatisticsResult, RegressionResult>;

struct ParsedNumberList {
    std::vector<double> values;
    std::string error;
    [[nodiscard]] bool valid() const noexcept { return error.empty(); }
};
struct ParsedMatrix {
    Matrix value;
    std::string error;
    [[nodiscard]] bool valid() const noexcept { return error.empty(); }
};

// Strict comma-separated finite decimal/scientific values. No silent partial parse.
// The ceiling is enforced before allocation; empty fields are errors.
[[nodiscard]] ParsedNumberList parseAdvancedNumbers(const std::string& text,
                                                    std::size_t limit = 100'000);
// Semicolon-separated rows, comma-separated columns. All rows rectangular.
[[nodiscard]] ParsedMatrix parseAdvancedMatrix(const std::string& text);
[[nodiscard]] const char* advancedValueType(const AdvancedMathValue& value) noexcept;

} // namespace rationalstudio
