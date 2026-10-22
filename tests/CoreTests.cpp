#include "Expression.hpp"
#include "Rational.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace rationalstudio;

namespace {

    int passed = 0;

    void check(
        bool ok,
        const std::string& what
    ) {

        if (!ok) {

            throw std::runtime_error(
                "FAIL: " + what
            );
        }

        ++passed;
    }

    void equal(
        const std::string& actual,
        const std::string& expected
    ) {

        check(
            actual == expected,
            "expected '" + expected +
            "', got '" + actual + "'"
        );
    }

    void closeTo(
        double actual,
        double expected,
        const std::string& name,
        double tolerance = 1e-10
    ) {

        check(
            std::isfinite(actual) &&
            std::abs(actual - expected) < tolerance,
              name
        );
    }

    void throws(
        const std::function<void()>& f,
                const std::string& name
    ) {

        try {

            f();

        } catch (const std::exception&) {

            ++passed;
            return;
        }

        throw std::runtime_error(
            "FAIL: expected exception for " + name
        );
    }

    void evaluates(
        const std::string& source,
        double expected,
        const std::string& label,
        double x = 0,
        double ans = 0,
        AngleMode mode = AngleMode::Radians
    ) {

        Expression expr(source);

        check(
            expr.valid(),
              label + " parse: " + expr.error()
        );

        closeTo(
            expr.evaluate(x, ans, mode),
                expected,
                label
        );
    }

} // namespace

int main() {

    try {

        // =====================================================
        // Rational-number arithmetic
        // =====================================================

        const Rational a =
        Rational::parse("5/2");

        const Rational b =
        Rational::parse("7/4");

        equal(
            (a + b).toString(),
              "17/4"
        );

        equal(
            (a - b).toString(),
              "3/4"
        );

        equal(
            (a * b).toString(),
              "35/8"
        );

        equal(
            (a / b).toString(),
              "10/7"
        );

        // Fraction normalization.

        equal(
            Rational::parse(" -42 / -56 ").toString(),
              "3/4"
        );

        equal(
            Rational::parse("42/-56").toString(),
              "-3/4"
        );

        equal(
            Rational::parse("0/-16").toString(),
              "0"
        );

        equal(
            Rational::parse("7").toString(),
              "7"
        );

        check(
            Rational::parse("3/4") ==
            Rational::parse("6/8"),

              "normalized equality"
        );

        check(
            Rational::parse("-7/4") <
            Rational::parse("3/8"),

              "negative comparison"
        );

        // Arbitrary-precision integer arithmetic.

        const auto huge = Rational::parse(
            "123456789012345678901234567890123456789/3"
        );

        equal(
            huge.toString(),
              "41152263004115226300411522630041152263"
        );

        // Decimal conversion and rounding.

        equal(
            Rational::parse("1/8").toDecimal(15),
              "0.125"
        );

        equal(
            Rational::parse("2/3").toDecimal(3),
              "0.667"
        );

        equal(
            Rational::parse("-99/100").toDecimal(0),
              "-1"
        );

        equal(
            Rational::parse("999/1000").toDecimal(2),
              "1"
        );

        equal(
            Rational::parse("1/3000").toDecimal(2),
              "0"
        );

        // Invalid fractions.

        throws(
            [] {
                Rational::parse("1/0");
            },
            "zero denominator"
        );

        throws(
            [] {
                Rational::parse("abc");
            },
            "malformed fraction"
        );

        throws(
            [] {
                Rational::parse("2/3/4");
            },
            "two slashes"
        );

        throws(
            [] {
                (void)(
                    Rational(1, 5) /
                    Rational(0, 1)
                );
            },
            "fraction division by zero"
        );

        // =====================================================
        // Scientific expression parsing
        // =====================================================

        evaluates(
            "2+3*4",
            14,
            "operator precedence"
        );

        evaluates(
            "(2+3)*4",
                  20,
                  "parentheses"
        );

        evaluates(
            "2^3^2",
            512,
            "right-associative power"
        );

        evaluates(
            "-2^2",
            -4,
            "negative power precedence"
        );

        evaluates(
            "(-2)^2",
                  4,
                  "parenthesized negative power"
        );

        evaluates(
            "2^-3",
            0.125,
            "negative exponent"
        );

        evaluates(
            "3!+2!",
            8,
            "factorial"
        );

        evaluates(
            "3!^2",
            36,
            "factorial power"
        );

        // Implicit multiplication.

        evaluates(
            "2(x+1)",
                  8,
                  "implicit multiplication",
                  3
        );

        evaluates(
            "2x+3",
            11,
            "variable x",
            4
        );

        // Trigonometric functions.

        evaluates(
            "sin(pi/2)",
                  1,
                  "trigonometry"
        );

        evaluates(
            "sin(90)",
                  1,
                  "degrees",
                  0,
                  0,
                  AngleMode::Degrees
        );

        Expression tangentPole("tan(90)");

        check(
            tangentPole.valid() &&
            std::isnan(
                tangentPole.evaluate(
                    0,
                    0,
                    AngleMode::Degrees
                )
            ),
            "tan of 90 degrees undefined"
        );

        evaluates(
            "asin(1)",
                  90,
                  "inverse degrees",
                  0,
                  0,
                  AngleMode::Degrees
        );

        // Logarithms and roots.

        evaluates(
            "ln(e)",
                  1,
                  "natural logarithm"
        );

        evaluates(
            "log(1000)",
                  3,
                  "common logarithm"
        );

        evaluates(
            "sqrt(25)+cbrt(8)",
                  7,
                  "roots"
        );

        // Additional implicit multiplication.

        evaluates(
            "2sin(pi/2)",
                  2,
                  "implicit times function"
        );

        evaluates(
            "sin(pi/2)cos(0)",
                  1,
                  "implicit times function pair"
        );

        // Multi-argument functions.

        evaluates(
            "max(3,7)-min(3,7)",
                  4,
                  "multi argument functions"
        );

        // Previous answer.

        evaluates(
            "ans*3",
            15,
            "previous answer",
            0,
            5
        );

        // Modulo.

        evaluates(
            "mod(13,5)",
                  3,
                  "modulo function"
        );

        // Unicode mathematical operators.

        evaluates(
            "2π",
            6.283185307179586,
            "unicode pi",
            0,
            0
        );

        evaluates(
            "3×4",
            12,
            "unicode multiplication"
        );

        // Trigonometric identity.

        evaluates(
            "sin(x)^2+cos(x)^2",
                  1,
                  "trig identity",
                  3.4
        );

        // =====================================================
        // Graph expressions
        // =====================================================

        Expression graph("x^2-4");

        check(
            graph.valid() &&
            graph.dependsOnX(),

              "graph identifies x"
        );

        closeTo(
            graph.evaluate(-2),
                0,
                "graph root"
        );

        // =====================================================
        // Malformed expressions
        // =====================================================

        for (
            const std::string& invalid : {
                "",
                "sin(",
             "bad(x)",
             "sqrt()",
             "min(3)",
             "3++",
             "1,2",
             "1=2",
             "3.2.4",
             "1e9999"
            }
        ) {

            Expression expr(invalid);

            check(
                !expr.valid(),
                  "reject malformed expression: " +
                  invalid
            );
        }

        // =====================================================
        // Mathematically undefined results
        // =====================================================

        for (
            const std::string& undefined : {
                "sqrt(-1)",
             "ln(0)",
             "1/0",
             "(-1)^0.5",
             "(-3)!"
            }
        ) {

            Expression expr(undefined);

            check(
                expr.valid(),
                  "undefined must parse: " +
                  undefined
            );

            check(
                std::isnan(
                    expr.evaluate()
                ),
                "undefined must evaluate to NaN: " +
                undefined
            );
        }

        std::cout
        << "PASS: "
        << passed
        << " assertions\n";

    } catch (const std::exception& e) {

        std::cerr
        << e.what()
        << '\n';

        return 1;
    }

    return 0;
}
