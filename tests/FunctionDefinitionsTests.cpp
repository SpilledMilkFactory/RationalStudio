#include "Expression.hpp"
#include "FunctionDefinitions.hpp"
#include "VariableDefinitions.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace rationalstudio;
namespace {
int checks = 0;
int failures = 0;
void check(bool truth, const std::string& why) {
    ++checks;
    if (!truth) { ++failures; std::cerr << "FAIL: " << why << '\n'; }
}
void near(double actual, double expected, const std::string& why) {
    check(std::isfinite(actual) && std::abs(actual - expected) < 1e-9, why);
}
Expression compile(const std::string& text, const FunctionDefinitions& defs,
                   const VariableScope& variables = {}) {
    return Expression(text, variables, defs.scope());
}
} // namespace

int main() {
    FunctionDefinitions functions;
    check(functions.define("f(x) = x^2 + 3").accepted, "define f(x)");
    check(functions.contains("F"), "case insensitive function name");
    check(functions.parameters("F") == std::vector<std::string>{"x"}, "one parameter");
    Expression basic = compile("f(2) + f(x)", functions);
    check(basic.valid(), "expression compiles calls");
    near(basic.evaluate(3), 19, "function invocation with x"); // 7 + 12
    check(basic.referencedFunctions() == std::vector<std::string>{"f"}, "function references");

    check(functions.define("g(t) = f(t) + 1").accepted, "g references f");
    check(functions.define("h(p, q) = g(p) + q").accepted, "two parameters and chaining");
    near(compile("h(2, 5)", functions).evaluate(), 13, "nested evaluation");
    check(functions.directFunctionDependencies("h") == std::vector<std::string>{"g"}, "direct caller deps");
    check(functions.directFunctionDependencies("g") == std::vector<std::string>{"f"}, "direct g deps");
    check(!compile("g(2, 3)", functions).valid(), "arity mismatch");
    check(!compile("g", functions).valid(), "call parentheses mandatory");
    check(!compile("unknown(2)", functions).valid(), "unknown function rejected");

    // A bad replacement may not alter any currently published definition.
    const auto before = functions.scope();
    check(!functions.define("f(x) = g(x) + 1").accepted, "cycle rejected");
    check(!functions.define("f(x,y) = x+y").accepted,
          "changing arity with callers rejects atomically");
    near(compile("h(2, 5)", functions).evaluate(), 13, "cycle preserved previous state");
    check(!functions.define("f(x) = f(x) + 1").accepted, "self recursion rejected");
    near(compile("f(2)", functions).evaluate(), 7, "self recursion rejected without change");
    check(!functions.define("f(x) = z(x) + 1").accepted, "forward call rejected");
    check(!functions.define("f(x) = 1 +").accepted, "malformed update rejected");
    check(!functions.define("f(x) = ans + x").accepted, "no implicit ans");
    check(!functions.define("bad(t) = t + x").accepted, "no undeclared x");
    check(!functions.define("f(x, x) = x").accepted, "duplicate parameter rejected");
    check(!functions.define("sin(x) = x").accepted, "builtin name reserved");
    check(!functions.define("pi(x) = x").accepted, "constant reserved");
    check(!functions.define("x(x) = x").accepted, "coordinate reserved");
    check(!functions.define("bad(a) = 1/0/0 + ").accepted, "bad empty rhs incomplete");
    check(!functions.define("bad() = 3").accepted, "at least one parameter");
    check(!functions.define("bad(t,) = 3").accepted, "reject empty parameter");
    check(!functions.define("bad(t) = 2 = 1").accepted, "reject double equal");
    check(!functions.define("bad(t) = bogus(t)").accepted, "unknown nested function");
    check(functions.names().size() == 3, "rejected edits don't insert");
    check(!functions.define("g(u) = f(u) + h(u,2)").accepted,
          "mutual recursive callers rejected");

    // Replacing f recompiles its direct and transitive dependents.
    auto edited = functions.define("f(x) = x^2 + 10");
    check(edited.accepted, "valid redefine");
    check(edited.recompiled == std::vector<std::string>({"f", "g", "h"}),
          "dependent compilation order");
    near(compile("h(2, 5)", functions).evaluate(), 20, "dependent function sees new f");
    // Already-compiled callers retain an immutable previous function snapshot.
    Expression oldView("g(2)", {}, before);
    check(oldView.valid(), "old function scope retained");
    near(oldView.evaluate(), 8, "old snapshot retains old bodies");
    check(!functions.erase("f").accepted, "cannot remove with dependents");
    check(!functions.erase("g").accepted, "cannot remove intermediate");
    check(functions.erase("h").accepted, "erase leaf");
    check(functions.erase("g").accepted, "erase dependent");
    check(functions.erase("f").accepted, "erase root");
    check(functions.names().empty(), "empty after removes");

    VariableDefinitions values;
    check(values.assign("a = 2").accepted, "define variable a");
    check(values.assign("b = a+3").accepted, "define dependent variable b");
    FunctionDefinitions coeffs(values.scope());
    check(coeffs.define("curve(x) = a*x^2 + b").accepted, "function captures variables");
    check(coeffs.define("wrap(u) = curve(u) + a").accepted, "function depends on another and vars");
    check(coeffs.variableDependencies("curve") == std::vector<std::string>({"a", "b"}),
          "variable references in curve");
    check(coeffs.variableDependencies("wrap") == std::vector<std::string>({"a", "b"}),
          "transitive variable references in wrapper");
    Expression graph = compile("wrap(x)", coeffs, values.scope());
    check(graph.valid(), "combined function and variable graph compiles");
    check(graph.referencedVariables() == std::vector<std::string>({"a", "b"}),
          "call propagates variable dependencies");
    near(graph.evaluate(3, 0, AngleMode::Radians, values.bindings()), 25,
         "combined expression initial"); // (2*9+5)+2
    check(values.assign("a = 4").accepted, "edit variable");
    near(graph.evaluate(3, 0, AngleMode::Radians, values.bindings()), 47,
         "variable refresh without recompiling consumer"); // (4*9+7)+4

    check(coeffs.define("shadow(a) = a^2 + b").accepted,
          "parameter shadows global variable");
    Expression shadow = compile("shadow(10)", coeffs, values.scope());
    check(shadow.valid(), "shadow body compiles");
    near(shadow.evaluate(0, 0, AngleMode::Radians, values.bindings()), 107,
         "lexical argument overrides global");
    check(coeffs.variableDependencies("shadow") == std::vector<std::string>{"b"},
          "parameter omitted from dependencies");
    check(!coeffs.define("a(t) = t").accepted, "function can't shadow scalar");
    check(coeffs.define("caller(a) = curve(a)").accepted, "caller parameter shadows ambient");
    near(compile("caller(2)", coeffs).evaluate(0,0,AngleMode::Radians,values.bindings()),
         23, "callee sees ambient a, not caller parameter a");
    check(coeffs.variableDependencies("caller") == std::vector<std::string>({"a", "b"}),
          "callee captured scalars survive shadowed formal arguments");
    Expression callerGraph = compile("caller(x)", coeffs, values.scope());
    check(callerGraph.referencedVariables() == std::vector<std::string>({"a", "b"}),
          "expression reports transitive captures for graph invalidation");

    // The function body itself may be undefined; callers receive NaN.
    check(coeffs.define("invalid(t) = sqrt(-1) + t").accepted, "undefined body compiles");
    check(std::isnan(compile("invalid(1)", coeffs).evaluate()), "undefined propagates");
    check(coeffs.define("trig(x) = sin(x)").accepted, "builtin functions usable");
    near(compile("trig(90)", coeffs).evaluate(0, 0, AngleMode::Degrees), 1,
         "angle mode preserved");

    // Multiple independent evaluations of the same compiled expression.
    Expression quadratic = compile("curve(x) + shadow(x)", coeffs, values.scope());
    check(quadratic.valid(), "combined expressions compiled");
    for (int n = -500; n <= 500; ++n) {
        const double x = n / 10.0;
        near(quadratic.evaluate(x, 0, AngleMode::Radians, values.bindings()),
             4*x*x + 7 + x*x + 7, "repeated multicallee evaluation");
    }

    // Large chain validates independent compilation and bounded call semantics.
    FunctionDefinitions chain;
    check(chain.define("f0(x) = x").accepted, "chain root");
    for (int i = 1; i <= 20; ++i) {
        check(chain.define("f" + std::to_string(i) + "(x) = f" +
                           std::to_string(i - 1) + "(x) + 1").accepted, "define chain");
    }
    near(compile("f20(10)", chain).evaluate(), 30, "deep chained call");
    check(!chain.define("f0(x) = f20(x)").accepted, "large cycle rejected");
    near(compile("f20(10)", chain).evaluate(), 30, "large cycle rollback");
    for (int i = 21; i < 64; ++i) {
        check(chain.define("f" + std::to_string(i) + "(x) = f" +
                           std::to_string(i - 1) + "(x) + 1").accepted,
              "bounded call-chain build");
    }
    near(compile("f63(0)", chain).evaluate(), 63, "deepest supported chain");
    check(!chain.define("f64(x) = f63(x) + 1").accepted,
          "limit call-stack depth and preserve prior definitions");
    check(!chain.contains("f64"), "depth rejection transactional");

    // Replacing the scalar universe is transactional and validates unresolved names.
    VariableScope empty;
    check(!coeffs.setVariableScope(empty).accepted, "cannot remove captured variables");
    near(compile("curve(3)", coeffs).evaluate(0,0,AngleMode::Radians,values.bindings()), 43,
         "scope rollback");

    std::cout << (failures ? "FAIL: " : "PASS: ") << checks
              << " FunctionDefinitions assertions" << std::endl;
    return failures ? 1 : 0;
}
