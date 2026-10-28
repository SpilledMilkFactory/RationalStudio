// Issue #36 — cross-owner regression coverage for variables, functions,
// dependency updates, invalid definitions, immutable scopes and dynamic values.
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
void check(bool condition, const std::string& label) {
    ++assertions;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << label << '\n';
    }
}
void close(double actual, double expected, const std::string& label) {
    check(std::isfinite(actual) && std::fabs(actual - expected) < 1e-9, label);
}
void close(std::optional<double> actual, double expected, const std::string& label) {
    check(actual && std::fabs(*actual - expected) < 1e-9, label);
}
bool includes(const std::vector<std::string>& values, const std::string& name) {
    return std::find(values.begin(), values.end(), name) != values.end();
}
} // namespace

int main() {
    VariableDefinitions values;
    check(values.assign(" A = 2 ").accepted, "declare scalar A");
    check(values.assign("b=a+3").accepted, "dependent scalar b");
    check(values.assign("c=b*2").accepted, "transitive scalar c");
    check(values.assign("unrelated=19").accepted, "independent scalar");
    check(values.names() == std::vector<std::string>({"a", "b", "c", "unrelated"}),
          "deterministic canonical scalar names");
    check(values.directDependencies("b") == std::vector<std::string>({"a"}),
          "direct dependency a->b");
    check(values.dependents("A") == std::vector<std::string>({"b", "c"}),
          "transitive dependencies a->b->c");
    close(values.value("C"), 10, "initial transitive value");

    FunctionDefinitions functions(values.scope());
    check(functions.define("f(t)=a*t+b").accepted, "define function capturing two scalars");
    check(functions.define("g(t)=f(t)+c").accepted, "define nested function");
    check(functions.define("h(t)=g(t)+f(t)").accepted, "define multiply nested function");
    check(functions.define("independent(t)=t^2+unrelated").accepted,
          "define unrelated function");
    check(functions.variableDependencies("g") == std::vector<std::string>({"a", "b", "c"}),
          "nested captures are transitive");
    check(functions.variableDependencies("independent") == std::vector<std::string>({"unrelated"}),
          "function captures stay isolated");
    check(functions.directFunctionDependencies("h") == std::vector<std::string>({"f", "g"}),
          "nested function dependency set");

    Expression consumer("g(x)+a", values.scope(), functions.scope());
    Expression nested("h(x)", values.scope(), functions.scope());
    Expression separate("independent(x)", values.scope(), functions.scope());
    check(consumer.valid() && nested.valid() && separate.valid(), "all graphs compile");
    close(consumer.evaluate(2, 0, AngleMode::Radians, values.bindings()), 21,
          "initial graph dynamic evaluation");
    close(nested.evaluate(2, 0, AngleMode::Radians, values.bindings()), 28,
          "initial nested graph evaluation");
    close(separate.evaluate(2, 0, AngleMode::Radians, values.bindings()), 23,
          "initial unrelated graph");
    check(graphCurveDependsOnAny(consumer, {"a"}), "direct and captured scalar recognized");
    check(graphCurveDependsOnAny(nested, {"c"}), "transitive captured scalar recognized");
    check(!graphCurveDependsOnAny(separate, {"a", "b", "c"}),
          "unrelated graph dependency isolation");

    const auto frozen = values.bindings();
    const auto separateRevision = values.revision("unrelated");
    auto changed = values.assign("a=4");
    check(changed.accepted, "dynamic root update accepted");
    check(changed.recomputed == std::vector<std::string>({"a", "b", "c"}),
          "dependency-first recomputation");
    check(changed.changed == std::vector<std::string>({"a", "b", "c"}),
          "transitive numerical changes reported");
    close(values.value("b"), 7, "b after a update");
    close(values.value("c"), 14, "c after a update");
    close(consumer.evaluate(2, 0, AngleMode::Radians, values.bindings()), 33,
          "compiled function consumer uses new bindings");
    close(consumer.evaluate(2, 0, AngleMode::Radians, frozen), 21,
          "captured immutable bindings remain unchanged");
    check(values.revision("unrelated") == separateRevision,
          "unrelated scalar revision not incremented");
    check(graphCurveDependsOnAny(consumer, changed.changed), "affected graph selected");
    check(!graphCurveDependsOnAny(separate, changed.changed), "unrelated graph not selected");
    close(separate.evaluate(2, 0, AngleMode::Radians, values.bindings()), 23,
          "unrelated function value unchanged");

    // Rejected edits must leave both owners and already compiled consumers valid.
    const auto bSource = values.source("b");
    const auto bRevision = values.revision("b");
    const auto oldFunctionScope = functions.scope();
    check(!values.assign("b=c+1").accepted, "indirect scalar cycle rejected");
    check(!values.assign("a=a+1").accepted, "self scalar cycle rejected");
    check(!values.assign("b=missing+1").accepted, "unknown scalar rejected");
    check(!values.assign("b=x+1").accepted, "graph coordinate in scalar rejected");
    check(!values.assign("b=ans+1").accepted, "previous answer in scalar rejected");
    check(!values.assign("sin=99").accepted, "reserved scalar rejected");
    check(values.source("b") == bSource && values.revision("b") == bRevision,
          "rejected scalar definitions preserve source and revision");
    close(values.value("c"), 14, "rejected scalar edits leave dependencies intact");
    check(!functions.define("f(t)=g(t)+1").accepted, "function indirect recursion rejected");
    check(!functions.define("f(t)=f(t)+1").accepted, "function self recursion rejected");
    check(!functions.define("g(t)=unknown(t)").accepted, "unknown function rejected");
    check(!functions.define("f(t)=t+").accepted, "syntax error rejected");
    check(!functions.define("f(t,t)=t").accepted, "duplicate formal rejected");
    check(!functions.define("f(t,u)=t+u").accepted,
          "breaking dependent call arity rejected");
    check(!functions.define("f(t)=ans+t").accepted, "previous answer rejected in function");
    check(functions.source("f") == "a*t+b", "rejected function updates preserve body");
    close(consumer.evaluate(2, 0, AngleMode::Radians, values.bindings()), 33,
          "compiled graph retained after invalid edits");
    close(Expression("h(2)", values.scope(), functions.scope()).evaluate(
          0, 0, AngleMode::Radians, values.bindings()), 44,
          "published function scope retained after invalid edits");

    const auto recompiled = functions.define("f(t)=a*t^2+b");
    check(recompiled.accepted, "valid nested function redefinition accepted");
    check(recompiled.recompiled == std::vector<std::string>({"f", "g", "h"}),
          "dependent functions recompiled in dependency-first order");
    Expression fresh("g(x)+a", values.scope(), functions.scope());
    check(fresh.valid(), "new function scope compiles");
    close(fresh.evaluate(2, 0, AngleMode::Radians, values.bindings()), 41,
          "new graph observes new function body");
    close(consumer.evaluate(2, 0, AngleMode::Radians, values.bindings()), 33,
          "previously compiled graph retains old function body");
    close(Expression("g(2)+a", values.scope(), oldFunctionScope).evaluate(
          0, 0, AngleMode::Radians, values.bindings()), 33,
          "old function scope is an immutable snapshot");
    check(!graphCurveDependsOnAny(fresh, {"unrelated"}), "fresh graph unaffected by unrelated scalar");

    // Undefined runtime math differs from invalid syntax and can recover.
    check(values.assign("q=1/(a-4)").accepted, "undefined-domain scalar accepted");
    check(!values.value("q"), "runtime undefined scalar value absent");
    check(values.assign("r=q+1").accepted, "undefined scalar dependent accepted");
    check(!values.value("r"), "dependent undefined propagates");
    check(functions.setVariableScope(values.scope()).accepted,
          "expanded scalar scope accepted without changing function bodies");
    check(functions.define("withdomain(t)=t+r").accepted,
          "function captures undefined scalar legally");
    Expression domain("withdomain(x)", values.scope(), functions.scope());
    check(domain.valid(), "undefined-domain function compiles");
    check(std::isnan(domain.evaluate(2, 0, AngleMode::Radians, values.bindings())),
          "undefined value propagates through function call");
    changed = values.assign("a=5");
    check(changed.accepted && includes(changed.changed, "q") && includes(changed.changed, "r"),
          "undefined-to-defined changes propagate through variables");
    close(values.value("q"), 1, "domain scalar recovered");
    close(values.value("r"), 2, "domain dependent recovered");
    close(domain.evaluate(2, 0, AngleMode::Radians, values.bindings()), 4,
          "same compiled function recovers with new bindings");
    changed = values.assign("a=4");
    check(changed.accepted && includes(changed.changed, "q") && includes(changed.changed, "r"),
          "defined-to-undefined changes propagated");
    check(std::isnan(domain.evaluate(2, 0, AngleMode::Radians, values.bindings())),
          "domain error returns without recompilation");

    // Lexical parameter shadows ambient scalar only within the function body.
    check(functions.define("shadow(a)=a+b").accepted, "parameter shadowing allowed");
    Expression lexical("shadow(10)", values.scope(), functions.scope());
    check(lexical.valid(), "shadow consumer compiles");
    close(lexical.evaluate(0, 0, AngleMode::Radians, values.bindings()), 17,
          "parameter value overrides ambient a");
    check(functions.variableDependencies("shadow") == std::vector<std::string>({"b"}),
          "shadowed variable not reported as capture");

    // Scope replacement failures must not corrupt the function publication.
    VariableScope insufficient;
    check(insufficient.declare("a"), "minimal scope declared");
    check(!functions.setVariableScope(insufficient).accepted,
          "reject dropping variables captured by functions");
    close(lexical.evaluate(0, 0, AngleMode::Radians, values.bindings()), 17,
          "compiled lexical caller remains valid after rejected scope");
    close(Expression("g(2)", values.scope(), functions.scope()).evaluate(
          0, 0, AngleMode::Radians, values.bindings()), 37,
          "published function remains after rejected scope");

    std::cout << (failures ? "FAIL: " : "PASS: ") << assertions
              << " VariableFunctionInteraction assertions\n";
    return failures ? 1 : 0;
}
