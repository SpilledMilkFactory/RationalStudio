#include "AdvancedMathInput.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <string>

using namespace rationalstudio;
namespace {
int assertions = 0;
int failures = 0;
void check(bool ok, const char* title) {
    ++assertions;
    if (!ok) { ++failures; std::cerr << "FAIL: " << title << '\n'; }
}
}
int main() {
    const auto values = parseAdvancedNumbers(" -2, 0.5 , 3e2 ");
    check(values.valid() && values.values.size() == 3, "finite list parsed");
    check(values.valid() && values.values[0] == -2 && values.values[1] == .5 &&
          values.values[2] == 300, "numeric values preserved");
    check(!parseAdvancedNumbers("").valid(), "empty rejected");
    check(!parseAdvancedNumbers("1,").valid(), "trailing comma rejected");
    check(!parseAdvancedNumbers(",1").valid(), "leading comma rejected");
    check(!parseAdvancedNumbers("1,,2").valid(), "missing field rejected");
    check(!parseAdvancedNumbers("1garbage").valid(), "trailing garbage rejected");
    check(!parseAdvancedNumbers("nan").valid(), "nan rejected");
    check(!parseAdvancedNumbers("inf").valid(), "infinity rejected");
    check(!parseAdvancedNumbers("1,2,3", 2).valid(), "element cap enforced");
    check(!parseAdvancedNumbers("1", 0).valid(), "invalid cap rejected");
    check(!parseAdvancedNumbers("1", 1000001).valid(), "unbounded cap rejected");
    const auto matrix = parseAdvancedMatrix("1, 2; 3, 4");
    check(matrix.valid() && matrix.value.rows == 2 && matrix.value.columns == 2,
          "matrix dimensions");
    check(matrix.valid() && matrix.value.at(1, 0) == 3, "matrix row-major entries");
    check(!parseAdvancedMatrix("1,2;3").valid(), "ragged rows rejected");
    check(!parseAdvancedMatrix("1,2;").valid(), "trailing matrix row rejected");
    check(!parseAdvancedMatrix(";1,2").valid(), "leading matrix row rejected");
    check(!parseAdvancedMatrix("1,2;;3,4").valid(), "empty matrix row rejected");
    check(!parseAdvancedMatrix("1,2;3,NaN").valid(), "nonfinite matrix input rejected");
    check(!parseAdvancedMatrix("1,2;3,4,5").valid(), "row width mismatch rejected");
    AdvancedMathValue a = 3.0;
    AdvancedMathValue b = ComplexValue{2, 1};
    AdvancedMathValue c = Matrix{};
    AdvancedMathValue d = MathVector{};
    AdvancedMathValue e = DescriptiveStatisticsResult{};
    AdvancedMathValue f = RegressionResult{};
    check(std::string(advancedValueType(a)) == "scalar", "scalar alternative retained");
    check(std::string(advancedValueType(b)) == "complex", "complex alternative retained");
    check(std::string(advancedValueType(c)) == "matrix", "matrix alternative retained");
    check(std::string(advancedValueType(d)) == "vector", "vector alternative retained");
    check(std::string(advancedValueType(e)) == "statistics", "statistics alternative retained");
    check(std::string(advancedValueType(f)) == "regression", "regression alternative retained");
    std::cout << (failures ? "FAIL: " : "PASS: ") << assertions << " AdvancedMathInput assertions\n";
    return failures ? 1 : 0;
}
