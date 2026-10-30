#include "InequalityRegion.hpp"
#include "Expression.hpp"
#include "MathExpressionInput.hpp"
#include <cmath>
#include <iostream>
#include <string>
using namespace rationalstudio;
namespace {
int checks = 0, fails = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++fails; std::cerr << "FAIL: " << label << '\n'; }
}
void parse(const std::string& input, bool above, bool inclusive, const std::string& expr) {
    const auto source = parseInequality(input);
    check(source.valid(), "valid comparison");
    check(source.shadeAbove == above, "comparison direction");
    check(source.inclusive == inclusive, "boundary inclusion");
    check(source.expression == expr, "scalar expression preserved");
}
}
int main() {
    parse("y > x^2", true, false, "x^2");
    parse("y >= x^2", true, true, "x^2");
    parse("y < x^2", false, false, "x^2");
    parse("y <= x^2", false, true, "x^2");
    parse("x^2 < y", true, false, "x^2");
    parse("x^2 <= y", true, true, "x^2");
    parse("x^2 > y", false, false, "x^2");
    parse("x^2 >= y", false, true, "x^2");
    parse(" y ≥ a*sin(x) ", true, true, "a*sin(x)");
    parse(" y ≤ 2x + 3 ", false, true, "2x + 3");
    for (const auto& input : {"", "x^2=3", "y==x", "y=x", "x<3", "y<y", "x<3<y",
                              "y<<x", "y>x>1", "y<", "<y", "y <= x >= 2"}) {
        check(!parseInequality(input).valid(), "invalid inequality rejected");
    }
    {
        Expression parabola(MathExpressionInput::normalize(parseInequality("y>x²").expression));
        check(parabola.valid(), "boundary uses existing math normalization");
        check(std::abs(parabola.evaluate(2, 0, AngleMode::Radians)-4.) < 1e-12, "analytical boundary");
    }
    for (const bool above : {true, false}) {
        for (double y : {-100., -10., -1., 0., 1., 10., 100.}) {
            const auto span = inequalityVerticalSpan(y, -5., 5., 100., above);
            if (above && y >= 5.) { check(!span.has_value(), "above none"); continue; }
            if (!above && y <= -5.) { check(!span.has_value(), "below none"); continue; }
            check(span.has_value(), "visible span exists");
            if (!span) continue;
            check(span->top >= 0 && span->bottom <= 100 && span->bottom > span->top,
                  "span clipped to visible height");
            if (y == 0.) check(std::abs((above ? span->bottom : span->top) - 50) < 1e-10, "analytical central boundary");
        }
    }
    for (const auto value : {NAN, INFINITY, -INFINITY}) {
        check(!inequalityVerticalSpan(value, -5, 5, 100, true), "invalid function value rejected");
        check(!inequalityVerticalSpan(0, -5, 5, value, false), "invalid viewport height rejected");
    }
    check(!inequalityVerticalSpan(0, 5, 5, 100, true), "zero y range rejected");
    check(!inequalityVerticalSpan(0, -5, 5, 0, false), "zero height rejected");
    const auto extreme = inequalityVerticalSpan(0, -1e300, 1e300, 200, true);
    check(extreme.has_value() && std::abs(extreme->bottom - 100.) < 1e-9,
          "extreme finite viewport remains stable");
    std::cout << (fails ? "FAIL: " : "PASS: ") << checks
              << " InequalityRegion assertions" << '\n';
    return fails ? 1 : 0;
}
