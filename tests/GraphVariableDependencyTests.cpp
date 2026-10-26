#include "Expression.hpp"
#include "FunctionDefinitions.hpp"
#include "GraphVariableDependencies.hpp"
#include "VariableDefinitions.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace rationalstudio;
namespace {
int assertions = 0;
void require(bool ok, const char* label) {
    ++assertions;
    if (!ok) throw std::runtime_error(label);
}
void near(double actual, double expected, const char* label) {
    require(std::isfinite(actual) && std::fabs(actual - expected) < 1e-9, label);
}
}
int main() {
    try {
        VariableDefinitions definitions;
        require(definitions.assign("a=2").accepted, "initial a");
        require(definitions.assign("b=a+3").accepted, "dependent b");
        require(definitions.assign("c=7").accepted, "unrelated c");
        const auto scope = definitions.scope();
        Expression direct("a*x", scope);
        Expression transitive("b*x", scope);
        Expression unrelated("c*x", scope);
        Expression builtin("sin(x)", scope);
        Expression invalid("unknown*x", scope);
        require(direct.valid() && transitive.valid() && unrelated.valid() && builtin.valid(),
                "compiled expressions valid");
        require(!invalid.valid(), "unknown identifier rejected");
        require(graphCurveDependsOnAny(direct, {"a"}), "direct dependency recognized");
        require(!graphCurveDependsOnAny(direct, {"b"}), "different variable unaffected");
        require(!graphCurveDependsOnAny(direct, {}), "empty changes unaffected");
        require(!graphCurveDependsOnAny(builtin, {"a", "b"}), "builtin unrelated");
        require(!graphCurveDependsOnAny(invalid, {"a"}), "invalid expression ignored");
        auto changed = definitions.assign("a=4");
        require(changed.accepted, "change accepted");
        require(graphCurveDependsOnAny(direct, changed.changed), "direct affected");
        require(graphCurveDependsOnAny(transitive, changed.changed), "transitive affected");
        require(!graphCurveDependsOnAny(unrelated, changed.changed), "unrelated unchanged");
        near(direct.evaluate(2, 0, AngleMode::Radians, definitions.bindings()), 8,
             "existing AST uses new binding");
        near(transitive.evaluate(2, 0, AngleMode::Radians, definitions.bindings()), 14,
             "transitive binding updated");
        FunctionDefinitions functions(scope);
        require(functions.define("f(t)=a*t+b").accepted, "capturing function");
        require(functions.define("g(t)=f(t)+1").accepted, "nested function");
        Expression indirect("g(x)", scope, functions.scope());
        require(indirect.valid(), "indirect expression valid");
        require(graphCurveDependsOnAny(indirect, {"a"}), "nested capture a recognized");
        require(graphCurveDependsOnAny(indirect, {"b"}), "nested capture b recognized");
        require(!graphCurveDependsOnAny(indirect, {"c"}), "unrelated c not captured");
        near(indirect.evaluate(2, 0, AngleMode::Radians, definitions.bindings()), 16,
             "nested function uses updated binding");
        for (int i = 0; i < 400; ++i) {
            auto result = definitions.assign("a=" + std::to_string(i));
            require(result.accepted, "reassignment accepted");
            require(graphCurveDependsOnAny(indirect, result.changed), "function capture affected");
            require(!graphCurveDependsOnAny(unrelated, result.changed), "unrelated remains clean");
        }
        std::cout << "PASS: " << assertions << " GraphVariableDependency assertions\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return 1;
    }
}
