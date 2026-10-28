// Issue #36 — deterministic, cross-component dynamic-evaluation oracle.
// This test deliberately calculates expected numbers independently of the
// Expression parser. All loops are bounded and use a fixed, reproducible seed.
#include "Expression.hpp"
#include "FunctionDefinitions.hpp"
#include "GraphVariableDependencies.hpp"
#include "VariableDefinitions.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

using namespace rationalstudio;
namespace {
int assertions = 0;
int failures = 0;
void check(bool value, const std::string& label) {
    ++assertions;
    if (!value) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
void close(double actual, double expected, const std::string& label) {
    check(std::isfinite(actual) && std::fabs(actual - expected) <=
          1e-9 * (1. + std::fabs(expected)), label);
}
void close(std::optional<double> actual, double expected, const std::string& label) {
    check(actual.has_value() && std::isfinite(*actual) &&
          std::fabs(*actual - expected) <= 1e-9 * (1. + std::fabs(expected)), label);
}
bool contains(const std::vector<std::string>& names, const std::string& candidate) {
    return std::find(names.begin(), names.end(), candidate) != names.end();
}
}
int main() {
    VariableDefinitions values;
    check(values.assign("a=0").accepted, "matrix a root");
    check(values.assign("b=2*a+1").accepted, "matrix b dependent");
    check(values.assign("c=b^2").accepted, "matrix c transitive");
    check(values.assign("d=c/(a+1)").accepted, "matrix d domain-sensitive");
    check(values.assign("independent=19").accepted, "matrix unrelated root");
    FunctionDefinitions functions(values.scope());
    check(functions.define("f(t)=a*t+b").accepted, "matrix function f");
    check(functions.define("g(t)=f(t)+c").accepted, "matrix function g");
    check(functions.define("u(t)=t*t+independent").accepted, "matrix unrelated function");
    Expression graph("g(x)+d", values.scope(), functions.scope());
    Expression fn("g(x)", values.scope(), functions.scope());
    Expression control("u(x)", values.scope(), functions.scope());
    check(graph.valid() && fn.valid() && control.valid(), "matrix compiled once");
    check(graphCurveDependsOnAny(graph, {"a", "b", "c", "d"}),
          "matrix graph sees captures");
    check(!graphCurveDependsOnAny(control, {"a", "b", "c", "d"}),
          "matrix unrelated graph isolated");

    const auto controlRevision = values.revision("independent");
    for (int iteration = 0; iteration < 240; ++iteration) {
        // Include a=-1 repeatedly to exercise defined/undefined transitions.
        const int a = (iteration * 7 % 17) - 8;
        const auto oldA = values.value("a");
        const auto oldB = values.value("b");
        const auto oldC = values.value("c");
        const auto oldD = values.value("d");
        const auto oldSnapshot = values.bindings();
        const double previousFunction = fn.evaluate(3, 0, AngleMode::Radians, oldSnapshot);
        const auto edit = values.assign("a=" + std::to_string(a));
        check(edit.accepted, "matrix root update accepted");
        check(edit.recomputed == std::vector<std::string>({"a", "b", "c", "d"}),
              "matrix exact affected subgraph and ordering");
        check(values.revision("independent") == controlRevision,
              "matrix isolated revision stable");
        const int b = 2*a+1;
        const int c = b*b;
        close(values.value("a"), a, "matrix root oracle");
        close(values.value("b"), b, "matrix first-degree oracle");
        close(values.value("c"), c, "matrix second-degree oracle");
        if (a == -1) {
            check(!values.value("d"), "matrix division-by-zero yields undefined");
        } else {
            close(values.value("d"), static_cast<double>(c)/(a+1), "matrix quotient oracle");
        }
        const auto hasValueChanged = [](std::optional<double> lhs,
                                        std::optional<double> rhs) { return lhs != rhs; };
        check(contains(edit.changed,"a") == hasValueChanged(oldA,values.value("a")),
              "matrix changed-name a exactness");
        check(contains(edit.changed,"b") == hasValueChanged(oldB,values.value("b")),
              "matrix changed-name b exactness");
        check(contains(edit.changed,"c") == hasValueChanged(oldC,values.value("c")),
              "matrix changed-name c exactness");
        check(contains(edit.changed,"d") == hasValueChanged(oldD,values.value("d")),
              "matrix changed-name d exactness");
        check(!contains(edit.changed,"independent"), "matrix unchanged name omitted");
        check(graphCurveDependsOnAny(graph,edit.changed), "matrix graph invalidates on change");
        check(!graphCurveDependsOnAny(control,edit.changed), "matrix unrelated graph stays clean");
        close(fn.evaluate(3,0,AngleMode::Radians,oldSnapshot), previousFunction,
              "matrix frozen binding snapshot remains stable");
        for (int step = -8; step <= 8; ++step) {
            const double x = step / 4.0;
            const double expectedG = a*x + b + c;
            close(fn.evaluate(x,0,AngleMode::Radians,values.bindings()), expectedG,
                  "matrix nested function numerical oracle");
            if (a == -1) {
                check(std::isnan(graph.evaluate(x,0,AngleMode::Radians,values.bindings())),
                      "matrix graph undefined propagation");
            } else {
                close(graph.evaluate(x,0,AngleMode::Radians,values.bindings()),
                      expectedG + static_cast<double>(c)/(a+1),
                      "matrix combined graph numerical oracle");
            }
            close(control.evaluate(x,0,AngleMode::Radians,values.bindings()),
                  x*x + 19, "matrix independent function oracle");
        }
        if (iteration % 19 == 0) {
            const auto priorRevision = values.revision("a");
            const auto beforeGraph = fn.evaluate(2,0,AngleMode::Radians,values.bindings());
            check(!values.assign("a=c+1").accepted, "matrix attempted scalar cycle rejected");
            check(!values.assign("b=undefined+1").accepted,
                  "matrix unknown identifier rejected");
            check(!functions.define("f(t)=g(t)+1").accepted,
                  "matrix attempted function recursion rejected");
            check(!functions.define("g(t)=f(t,t)").accepted,
                  "matrix malformed arity rejected");
            check(values.revision("a") == priorRevision,
                  "matrix rejected edits do not mutate revision");
            close(fn.evaluate(2,0,AngleMode::Radians,values.bindings()),
                  beforeGraph, "matrix invalid edits leave compiled function intact");
        }
    }

    // Angle-mode transitions require dynamic reevaluation of existing scalar
    // expressions, without changing their sources or requiring graph reparsing.
    check(values.assign("angle=sin(90)").accepted, "angle scalar defined");
    check(functions.setVariableScope(values.scope()).accepted,
          "function owner accepts extended scalar universe");
    check(functions.define("angled(t)=t+angle").accepted, "angle capturing function");
    Expression angled("angled(x)", values.scope(), functions.scope());
    check(angled.valid(), "angle consumer compiles");
    close(angled.evaluate(2,0,AngleMode::Radians,values.bindings()),
          2+std::sin(90.), "radian scalar oracle");
    auto change = values.setAngleMode(AngleMode::Degrees);
    check(change.accepted && contains(change.changed,"angle"),
          "degree update reports angle scalar change");
    close(angled.evaluate(2,0,AngleMode::Degrees,values.bindings()), 3,
          "degree scalar and function oracle");
    check(!graphCurveDependsOnAny(control,change.changed),
          "angle scalar does not affect unrelated control curve");
    change = values.setAngleMode(AngleMode::Degrees);
    check(change.accepted && change.recomputed.empty() && change.changed.empty(),
          "same-angle operation is a no-op");
    change = values.setAngleMode(AngleMode::Radians);
    check(change.accepted && contains(change.changed,"angle"),
          "return to radians reports numerical change");
    close(angled.evaluate(2,0,AngleMode::Radians,values.bindings()),
          2+std::sin(90.), "radian restoration oracle");

    std::cout << (failures ? "FAIL: " : "PASS: ") << assertions
              << " DynamicEvaluationMatrix assertions\n";
    return failures ? 1 : 0;
}
