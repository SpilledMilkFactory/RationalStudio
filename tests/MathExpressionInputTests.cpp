#include "Expression.hpp"
#include "MathExpressionInput.hpp"

#include <cmath>
#include <iostream>
#include <string>

using namespace rationalstudio;
namespace {
    int checks=0, failures=0;
    void check(bool ok, const char* what) { ++checks; if(!ok) { ++failures; std::cerr << "FAIL: " << what << '\n'; } }
    void value(std::string raw, double expected) {
        const auto text = MathExpressionInput::normalize(raw);
        Expression expression(text);
        check(expression.valid(), "normalized parse valid");
        if (expression.valid()) check(std::abs(expression.evaluate(0, 0, AngleMode::Radians)-expected)<1e-10, "normalized value");
    }
}
int main() {
    check(MathExpressionInput::normalize("π") == "pi", "pi");
    check(MathExpressionInput::normalize("τ") == "tau", "tau");
    check(MathExpressionInput::normalize("3×4") == "3*4", "multiply");
    check(MathExpressionInput::normalize("8÷2") == "8/2", "divide");
    check(MathExpressionInput::normalize("5−2") == "5-2", "minus");
    check(MathExpressionInput::normalize("x²+x³") == "x^2+x^3", "superscripts");
    check(MathExpressionInput::normalize("√(9)") == "sqrt(9)", "radical");
    check(MathExpressionInput::normalize("2·3∙4") == "2*3*4", "centered products");
    check(MathExpressionInput::normalize("sin(x)") == "sin(x)", "ASCII unchanged");
    check(MathExpressionInput::looksIncomplete("sin("), "open call incomplete");
    check(MathExpressionInput::looksIncomplete("2+"), "operator incomplete");
    check(MathExpressionInput::looksIncomplete("max(2,"), "separator incomplete");
    check(!MathExpressionInput::looksIncomplete("sin(x)"), "balanced complete");
    check(!MathExpressionInput::looksIncomplete("2^3"), "power complete");
    // ---------------------------------------------------------
    // Issue #32 — Invalid versus incomplete operator sequences
    // ---------------------------------------------------------

    check(
        !MathExpressionInput::looksIncomplete("2+*"),
          "adjacent binary operators invalid"
    );

    check(
        !MathExpressionInput::looksIncomplete("2 + * "),
          "spaced binary operators invalid"
    );

    check(
        !MathExpressionInput::looksIncomplete("2**"),
          "repeated multiplication invalid"
    );

    check(
        !MathExpressionInput::looksIncomplete("2//3"),
          "repeated division invalid"
    );

    check(
        !MathExpressionInput::looksIncomplete("sin(*"),
          "binary operator after opening invalid"
    );

    check(
        !MathExpressionInput::looksIncomplete("x^/"),
          "binary operator after exponent invalid"
    );

    check(
        MathExpressionInput::looksIncomplete("x*"),
          "single trailing binary operator incomplete"
    );

    check(
        MathExpressionInput::looksIncomplete("sin(2*"),
          "unclosed call with trailing operator incomplete"
    );
    value("√(9)+2", 5);
    value("3×4÷2", 6);
    value("5−2", 3);
    value("2²+2³", 12);
    value("sin(π/2)", 1);
    for (int i=1; i<=2000; ++i) {
        const std::string s=std::to_string(i)+"×2+π";
        const std::string norm=MathExpressionInput::normalize(s);
        Expression expression(norm);
        check(expression.valid(), "Unicode sweep parse");
        if(expression.valid()) check(std::abs(expression.evaluate(0,0,AngleMode::Radians)-(2*i+3.14159265358979323846))<1e-9,"Unicode sweep evaluation");
    }
    std::cout << (failures?"FAIL: ":"PASS: ") << checks << " MathExpressionInput assertions\n";
    return failures?1:0;
}
