#include "VariableDefinitions.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace rationalstudio;
namespace {
int assertions = 0;
int failures = 0;
void check(bool value, const std::string& label) {
    ++assertions;
    if (!value) {
        ++failures;
        std::cerr << "FAIL: " << label << '\n';
    }
}
void near(std::optional<double> actual, double expected, const std::string& label) {
    check(actual && std::abs(*actual - expected) < 1e-10, label);
}
void near(double actual, double expected, const std::string& label) {
    check(std::isfinite(actual) && std::abs(actual - expected) < 1e-10, label);
}
void same(const std::vector<std::string>& a,
          const std::vector<std::string>& b, const std::string& label) {
    check(a == b, label);
}
void rejected(const VariableUpdate& edit, const std::string& part) {
    check(!edit.accepted && edit.error.find(part) != std::string::npos,
          "expected rejection containing: " + part + " [" + edit.error + "]");
}
}

int main() {
    VariableDefinitions env;
    check(env.names().empty(), "empty environment");
    rejected(env.assign("a"), "one assignment");
    rejected(env.assign("a==2"), "one assignment");
    rejected(env.assign("=2"), "Identifier");
    rejected(env.assign("a="), "empty");
    rejected(env.assign("x=3"), "reserved");
    rejected(env.assign("sin=3"), "reserved");
    rejected(env.assign("2a=3"), "Identifier");
    rejected(env.assign("a = unknown + 1"), "Unknown identifier");
    rejected(env.assign("a = a + 1"), "Circular");
    rejected(env.assign("a = x+1"), "x or ans");
    rejected(env.assign("a = ans+1"), "x or ans");
    check(env.names().empty(), "failures leave empty environment intact");

    auto r = env.assign(" A = 2 ");
    check(r.accepted && r.error.empty(), "define A");
    same(r.recomputed, {"a"}, "recomputed a");
    same(r.changed, {"a"}, "changed a");
    near(env.value("a"), 2, "a=2");
    near(env.value("A"), 2, "case-insensitive value");
    check(env.contains("a") && env.contains("A"), "case-insensitive contains");
    same(env.names(), {"a"}, "canonical names");
    check(env.source("a") == "2", "source recorded");

    check(env.assign("b = 3*a + 1").accepted, "define b");
    check(env.assign("c = b^2").accepted, "define c");
    check(env.assign("unrelated = 100").accepted, "define unrelated");
    near(env.value("b"), 7, "b depends on a");
    near(env.value("c"), 49, "c depends transitively on a");
    same(env.directDependencies("b"), {"a"}, "direct dep b");
    same(env.directDependencies("c"), {"b"}, "direct dep c");
    same(env.dependents("a"), {"b", "c"}, "transitive dependents a");
    same(env.dependents("unrelated"), {}, "unrelated no dependents");

    Expression graph("c*x + unrelated", env.scope());
    check(graph.valid(), "external graph expression compiles");
    near(env.evaluate(graph, 2), 198, "external graph with current values");
    const auto oldUnrelatedRevision = env.revision("unrelated");
    const auto oldBRevision = env.revision("b");
    const auto oldCRevision = env.revision("c");
    r = env.assign("a=4");
    check(r.accepted, "reassign a");
    same(r.recomputed, {"a", "b", "c"}, "ordered affected subgraph");
    same(r.changed, {"a", "b", "c"}, "all changed values");
    near(env.value("b"), 13, "b updated");
    near(env.value("c"), 169, "c updated");
    near(env.evaluate(graph, 2), 438, "compiled graph uses fresh bindings");
    check(env.revision("unrelated") == oldUnrelatedRevision,
          "unrelated node not revised");
    check(env.revision("b") > oldBRevision && env.revision("c") > oldCRevision,
          "dependent revisions updated");

    r = env.assign("a=4+0");
    check(r.accepted, "equivalent change accepted");
    same(r.recomputed, {"a", "b", "c"}, "recomputed even for same result");
    same(r.changed, {}, "same finite results do not invalidate clients");

    const auto bSource = env.source("b");
    const auto bRevision = env.revision("b");
    rejected(env.assign("b=c+1"), "Circular");
    check(env.source("b") == bSource && env.revision("b") == bRevision,
          "cycle rejection is transactional");
    near(env.value("c"), 169, "cycle rejection keeps results");
    rejected(env.assign("a=c+1"), "Circular");
    rejected(env.assign("c=c+1"), "Circular");
    rejected(env.assign("b=nonexistent+1"), "Unknown identifier");
    rejected(env.assign("b=sin("), "Expected");
    rejected(env.assign("b=x+2"), "x or ans");
    rejected(env.assign("b=ans*2"), "x or ans");
    near(env.value("b"), 13, "all invalid updates preserve value");

    r = env.assign("b=5");
    same(r.recomputed, {"b", "c"}, "dependents after b redefinition");
    same(env.directDependencies("b"), {}, "b no longer depends on a");
    near(env.value("c"), 25, "c recomputed from new b");
    const auto revC = env.revision("c");
    r = env.assign("a=10");
    same(r.recomputed, {"a"}, "old dependent edges removed");
    check(env.revision("c") == revC, "c unchanged after detached a");
    near(env.value("c"), 25, "c still 25");

    rejected(env.erase("b"), "referenced");
    rejected(env.erase("missing"), "Unknown");
    r = env.erase("unrelated");
    check(r.accepted, "remove isolated variable");
    same(r.changed, {"unrelated"}, "remove affects external graph name");
    check(!env.contains("unrelated"), "removed absent");
    check(std::isnan(env.evaluate(graph, 1)), "old graph sees missing binding");
    check(!Expression("unrelated+1", env.scope()).valid(),
          "future compiles cannot use removed name");
    check(env.assign("unrelated=50").accepted, "recreate removed name");
    near(env.evaluate(graph, 2), 100, "old graph becomes defined again");

    // Undefined values propagate through the DAG; a later edit repairs them.
    check(env.assign("q=1/(a-10)").accepted, "undefined expression is valid definition");
    check(!env.value("q"), "q initially mathematically undefined");
    check(env.assign("r=q+1").accepted, "undefined source referenced legally");
    check(!env.value("r"), "dependent is undefined");
    double unused = 0;
    check(!env.bindings().lookup("q", unused),
          "undefined values not published to bindings");
    r = env.assign("a=12");
    same(r.recomputed, {"a", "q", "r"}, "undefined chain propagation order");
    near(env.value("q"), .5, "q repaired");
    near(env.value("r"), 1.5, "r repaired");
    same(r.changed, {"a", "q", "r"}, "undefined-to-defined counts as changed");
    r = env.assign("a=10");
    check(r.accepted && !env.value("q") && !env.value("r"),
          "defined-to-undefined propagates");

    // Changes in angle mode recompute definitions without recompiling them.
    check(env.assign("t = sin(90)").accepted, "trig expression defined");
    r = env.setAngleMode(AngleMode::Degrees);
    check(r.accepted, "degree mode accepted");
    near(env.value("t"), 1, "degree mode trig");
    same(env.setAngleMode(AngleMode::Degrees).recomputed, {},
         "unchanged angle mode does not recompute");
    r = env.setAngleMode(AngleMode::Radians);
    near(env.value("t"), std::sin(90.0), "radian mode trig");
    check(std::find(r.recomputed.begin(), r.recomputed.end(), "t") !=
          r.recomputed.end(), "angle mode recomputed t");

    // A long dependency chain must not touch an unrelated node on update.
    VariableDefinitions longChain;
    check(longChain.assign("v0=1").accepted, "chain root");
    for (int i = 1; i <= 64; ++i) {
        const auto stmt = "v" + std::to_string(i) + " = v" +
                          std::to_string(i-1) + " + 1";
        check(longChain.assign(stmt).accepted, "chain assignment " + stmt);
    }
    check(longChain.assign("independent=777").accepted, "independent chain var");
    const auto unchanged = longChain.revision("independent");
    for (int k = 0; k < 200; ++k) {
        r = longChain.assign("v0=" + std::to_string(k));
        check(r.accepted, "chain mutation accepted");
        check(r.recomputed.size() == 65, "65 nodes recomputed");
        near(longChain.value("v64"), k + 64., "chain last result");
        check(longChain.revision("independent") == unchanged,
              "independent never reevaluated");
    }
    near(longChain.value("v64"), 263, "final chain result");

    std::cout << (failures ? "FAIL: " : "PASS: ") << assertions
              << " VariableDefinitions assertions\n";
    return failures ? 1 : 0;
}
