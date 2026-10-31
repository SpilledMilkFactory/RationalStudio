#include "ComplexMath.hpp"
#include "FunctionDefinitions.hpp"

#include <cmath>
#include <complex>
#include <iostream>
#include <limits>
#include <numbers>
#include <random>
#include <string>

using namespace rationalstudio;
namespace {
int checks = 0, failures = 0;
void check(bool condition, const std::string& what) {
    ++checks;
    if (!condition) { ++failures; std::cerr << "FAIL: " << what << '\n'; }
}
bool close(ComplexValue lhs, ComplexValue rhs, double tolerance = 1e-10) {
    return finiteComplex(lhs) && finiteComplex(rhs) &&
           std::abs(lhs - rhs) <= tolerance * (1.0 + std::abs(rhs));
}
void oracle(const std::string& expression, ComplexValue expected, double tolerance = 1e-10,
            AngleMode mode = AngleMode::Radians) {
    ComplexExpression e(expression);
    check(e.valid(), expression + " parsed: " + e.error());
    const auto r = e.evaluate({}, {}, mode);
    check(r.successful(), expression + " evaluated: " + r.message);
    check(r.successful() && close(r.value, expected, tolerance), expression + " reference");
}
}
int main() {
    constexpr double pi = std::numbers::pi;
    oracle("3+4i", {3,4});
    oracle("(3+4i)+(1-2i)", {4,2});
    oracle("(3+4i)-(1-2i)", {2,6});
    oracle("(3+4i)*(1-2i)", {11,-2});
    oracle("(3+4i)/(1-2i)", {-1,2});
    oracle("i^2", {-1,0});
    oracle("(-1)^0.5", {0,1});
    oracle("sqrt(-1)", {0,1});
    oracle("sqrt(-4)", {0,2});
    oracle("exp(i*pi)", {-1,0});
    oracle("ln(-1)", {0,pi});
    oracle("log(100)", {2,0});
    oracle("log10(100)", {2,0});
    oracle("sin(i)", {0,std::sinh(1)});
    oracle("cos(i)", {std::cosh(1),0});
    oracle("sinh(i)", {0,std::sin(1)});
    oracle("cosh(i)", {std::cos(1),0});
    oracle("conj(3+4i)", {3,-4});
    oracle("real(3+4i)", {3,0});
    oracle("imag(3+4i)", {4,0});
    oracle("norm(3+4i)", {25,0});
    oracle("abs(3+4i)", {5,0});
    oracle("arg(i)", {pi/2,0});
    oracle("pow(i,2)", {-1,0});
    oracle("-2^2", {-4,0});
    oracle("2^3^2", {512,0});
    oracle("2(3+i)", {6,2});
    oracle("sin(30)", {.5,0}, 1e-12, AngleMode::Degrees);
    oracle("asin(0.5)", {30,0}, 1e-12, AngleMode::Degrees);
    oracle("arg(i)", {90,0}, 1e-12, AngleMode::Degrees);
    oracle("sin(i*90)", {0,std::sinh(pi/2)}, 1e-11, AngleMode::Degrees);
    oracle("exp(0)", {1,0});
    oracle("(1+i)^2", {0,2});
    oracle("tau/pi", {2,0});
    oracle("E^0", {1,0});
    oracle("2I", {0,2});
    oracle("sin(pi/2)", {1,0});

    check(formatComplex({3,4}) == "3 + 4i", "format rectangular positive");
    check(formatComplex({3,-4}) == "3 - 4i", "format rectangular negative");
    check(formatComplex({0,1}) == "i", "format positive imaginary unit");
    check(formatComplex({0,-1}) == "-i", "format negative imaginary unit");
    check(formatComplex({0,2}) == "2i", "format pure imaginary");
    check(formatComplex({-3,0}) == "-3", "format real negative");
    check(formatComplex({-0.0, -0.0}) == "0", "format signed zeros");
    check(formatComplex({1.0,2.0}, 0) == "invalid precision", "invalid format precision");
    check(formatComplex({std::numeric_limits<double>::infinity(),0}) == "undefined",
          "format infinity rejected");
    check(!finiteComplex({std::numeric_limits<double>::quiet_NaN(),0}), "nan not finite");
    check(std::string(complexStatusName(ComplexStatus::UnsupportedOperation)) ==
          "unsupported operation", "status description");

    for (const auto& s : {"", "2+", "sqrt(", "unknown(2)", "q+1", "1/0!", "2%3",
                           "i!", "i+1=2", "sin()", "pow(1)", "1e9999"}) {
        ComplexExpression e(s);
        check(!e.valid(), "invalid syntax rejected: " + std::string(s));
        check(!e.error().empty(), "invalid syntax has diagnostics");
        check(e.evaluate().status == ComplexStatus::InvalidExpression,
              "invalid expression no evaluation result");
    }
    {
        ComplexExpression e("1/0");
        check(e.valid(), "divide by zero is parseable");
        check(e.evaluate().status == ComplexStatus::UndefinedValue,
              "divide by zero runtime status");
    }
    {
        ComplexExpression e("exp(1000)");
        check(e.evaluate().status == ComplexStatus::UndefinedValue, "overflow status");
    }
    {
        ComplexExpression e("ln(0)");
        check(e.evaluate().status == ComplexStatus::UndefinedValue, "log zero status");
    }
    {
        ComplexExpression e("sqrt(z) + ans");
        auto r = e.evaluate({-4,0}, {3,1});
        check(r.successful() && close(r.value, {3,3}), "complex z and ans");
        check(e.evaluate({std::numeric_limits<double>::infinity(), 0}).status ==
              ComplexStatus::UndefinedValue, "invalid complex argument");
    }
    {
        ComplexExpression e("x^2+z");
        auto r = e.evaluate({2,3});
        check(r.successful() && close(r.value, {6,3}), "x uses real coordinate of z");
    }
    {
        VariableScope scope;
        check(scope.declare("a"), "real variable declared");
        VariableBindings b;
        check(b.set("a", 2), "real variable bound");
        ComplexExpression e("a*z + i", scope);
        auto first = e.evaluate({2,3}, {}, AngleMode::Radians, b);
        check(first.successful() && close(first.value, {4,7}), "real scalar multiplies complex z");
        check(b.set("a", 3), "real variable rebound");
        auto second = e.evaluate({2,3}, {}, AngleMode::Radians, b);
        check(second.successful() && close(second.value, {6,10}), "new real binding snapshot");
        VariableBindings missing;
        check(e.evaluate({}, {}, AngleMode::Radians, missing).status ==
              ComplexStatus::UndefinedValue, "missing named variable rejected");
    }
    {
        FunctionDefinitions f;
        check(f.define("square(t)=t^2").accepted, "compiled real user function defined");
        VariableScope vars;
        ComplexExpression e("square(3)+i", vars, f.scope());
        auto r = e.evaluate();
        check(e.valid() && r.successful() && close(r.value, {9,1}),
              "existing compiled real function used from complex expression");
        ComplexExpression notComplex("square(i)", vars, f.scope());
        check(notComplex.valid(), "complex user function call syntax accepted");
        check(notComplex.evaluate().status == ComplexStatus::UnsupportedOperation,
              "real user function rejects complex argument explicitly");
    }
    {
        Expression expr("sin(x)");
        VariableBindings b;
        auto r = evaluateComplex(expr, pi/2, 0, AngleMode::Radians, b);
        check(r.successful() && close(r.value, {1,0}), "legacy real expression promoted");
        Expression invalid("badname(x)");
        check(evaluateComplex(invalid,0,0,AngleMode::Radians,b).status ==
              ComplexStatus::InvalidExpression, "invalid real expression not promoted");
        Expression domain("sqrt(x)");
        check(evaluateComplex(domain,-1,0,AngleMode::Radians,b).status ==
              ComplexStatus::UndefinedValue, "legacy real domain stays real (sqrt negative)");
        check(domain.evaluate(-1) != domain.evaluate(-1), "legacy real sqrt(-1) unchanged");
    }
    {
        // Reproducible finite reference identities: exact oracle arithmetic,
        // not merely comparison with the same evaluator under test.
        std::mt19937_64 gen(0x49C0FFEEULL);
        std::uniform_int_distribution<int> d(-8,8);
        for (int j = 0; j < 120; ++j) {
            const int a = d(gen), b = d(gen), c = d(gen), e = d(gen);
            const std::string left = "(" + std::to_string(a) + "+(" + std::to_string(b) + ")i)";
            const std::string right = "(" + std::to_string(c) + "+(" + std::to_string(e) + ")i)";
            const auto sum = ComplexExpression(left + "+" + right).evaluate();
            check(sum.successful() && close(sum.value, {double(a+c),double(b+e)}),
                  "fixed-seed exact addition oracle");
            const auto product = ComplexExpression(left + "*" + right).evaluate();
            check(product.successful() &&
                  close(product.value, {double(a*c-b*e),double(a*e+b*c)}),
                  "fixed-seed exact multiplication oracle");
            const auto quotient = ComplexExpression(left + "/" + right).evaluate();
            if (c == 0 && e == 0) {
                check(quotient.status == ComplexStatus::UndefinedValue,
                      "fixed-seed zero denominator rejected");
            } else {
                const double denominator = double(c*c + e*e);
                check(quotient.successful() &&
                      close(quotient.value, {double(a*c+b*e)/denominator,
                                             double(b*c-a*e)/denominator}),
                      "fixed-seed exact division oracle");
            }
        }
    }
    std::cout << (failures ? "FAIL: " : "PASS: ") << checks
              << " ComplexMath assertions\n";
    return failures ? 1 : 0;
}
