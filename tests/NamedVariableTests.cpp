#include "Expression.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace rationalstudio;

namespace {

    int checks = 0;
    int fails = 0;

    void check(bool good, const std::string& label) {
        ++checks;

        if (!good) {
            ++fails;
            std::cerr << "FAIL: " << label << '\n';
        }
    }

    void near(double actual, double expected, const std::string& label) {
        check(
            std::isfinite(actual) &&
            std::abs(actual - expected) < 1e-10,
              label
        );
    }

} // namespace

int main() {
    VariableScope scope;
    std::string error;

    check(
        scope.declare("a", &error) && error.empty(),
          "declare a"
    );

    check(scope.declare("B", &error), "declare B");
    check(scope.declare("_offset9", &error), "leading underscore");
    check(scope.declare("Scale_2", &error), "alphanumeric underscore");

    check(
        scope.contains("A") && scope.contains("b"),
          "case-insensitive lookup"
    );

    check(scope.contains("scale_2"), "canonical name");

    check(
        !scope.declare("A", &error) &&
        error.find("Duplicate") != std::string::npos,
          "duplicate case insensitive"
    );

    check(
        scope.names().count("a") == 1 &&
        scope.names().count("b") == 1,
          "scope canonical storage"
    );

    for (const auto& reserved : {
        "x", "X", "ans", "ANS", "pi", "tau",
         "e", "sin", "atan2", "pow", "mod", "floor"
    }) {
        check(
            !scope.declare(reserved, &error) && !error.empty(),
              std::string("reserved ") + reserved
        );
    }

    for (const auto& invalid : {
        "", "9scale", "white space", "foo-bar",
         "a.b", "a\xC3\xA9", "a[b]", "two\twords", "a=b"
    }) {
        check(
            !scope.declare(invalid, &error),
              std::string("invalid ") + invalid
        );
    }

    check(
        !scope.declare(std::string(65, 'a'), &error),
          "name too long"
    );

    check(
        scope.declare(std::string(64, 'z')),
          "max-length name"
    );

    VariableBindings bindings;

    check(bindings.set("A", 2), "bind uppercase A");
    check(bindings.set("b", 3), "bind b");
    check(bindings.set("_offset9", -4), "bind offset");
    check(bindings.set("Scale_2", 1.5), "bind mixed case");

    check(
        !bindings.set("x", 8) &&
        !bindings.set("pi", 8) &&
        !bindings.set("cos", 8),
          "cannot override builtins"
    );

    check(
        !bindings.set(
            "a",
            std::numeric_limits<double>::infinity()
        ),
        "reject nonfinite inf"
    );

    check(
        !bindings.set(
            "a",
            std::numeric_limits<double>::quiet_NaN()
        ),
        "reject nonfinite nan"
    );

    double value = 0;

    check(
        bindings.lookup("A", value) && value == 2,
          "case insensitive get"
    );

    Expression legacy("sin(pi/2) + x^2 + ans");

    check(
        legacy.valid() && legacy.dependsOnX(),
          "legacy expression"
    );

    near(legacy.evaluate(2, 4), 9, "legacy evaluation");

    check(
        !Expression("a*x + b").valid(),
          "legacy unknown variable remains rejected"
    );

    check(
        !Expression("sin + 1").valid(),
          "builtin must be a call"
    );

    check(
        !Expression("sinn(x)").valid(),
          "unknown builtin fails"
    );

    Expression parabola("a*x^2 + b", scope);

    check(
        parabola.valid() && parabola.dependsOnX(),
          "compiled parabola"
    );

    check(
        parabola.referencedVariables() ==
        std::vector<std::string>({"a", "b"}),
          "sorted distinct references"
    );

    near(
        parabola.evaluate(3, 0, AngleMode::Radians, bindings),
         21,
         "parabola initial"
    );

    check(
        std::isnan(parabola.evaluate(3)),
          "unbound expression gives nan"
    );

    check(
        bindings.set("a", 4) && bindings.set("b", -2),
          "change values"
    );

    near(
        parabola.evaluate(3, 0, AngleMode::Radians, bindings),
         34,
         "rebind without reparse"
    );

    near(
        parabola.evaluate(-2, 0, AngleMode::Radians, bindings),
         14,
         "same expression different x"
    );

    Expression withoutX("b + _offset9 + Scale_2", scope);

    check(
        withoutX.valid() && !withoutX.dependsOnX(),
          "non x dependent"
    );

    check(
        withoutX.referencedVariables() ==
        std::vector<std::string>({"_offset9", "b", "scale_2"}),
          "case folded references"
    );

    near(
        withoutX.evaluate(
            77, 5, AngleMode::Degrees, bindings
        ),
        -4.5,
        "mixed variables"
    );

    Expression implicit("2a+3(b+1)", scope);

    check(implicit.valid(), "implicit variable multiply");

    near(
        implicit.evaluate(
            0, 0, AngleMode::Radians, bindings
        ),
        8 + 3 * (-1),
         "implicit evaluated"
    );

    Expression degree("sin(90) * a + ans", scope);

    check(degree.valid(), "builtin combined named");

    near(
        degree.evaluate(
            0, 7, AngleMode::Degrees, bindings
        ),
        11,
        "angle and ans preserved"
    );

    Expression repeated("a*a + b + a", scope);

    check(
        repeated.referencedVariables() ==
        std::vector<std::string>({"a", "b"}),
          "distinct references"
    );

    Expression constant("pi+e+tau", scope);

    check(
        constant.valid() &&
        constant.referencedVariables().empty(),
          "constants no bindings"
    );

    Expression reservedOverride("sin(0) + max(2,3)", scope);

    check(
        reservedOverride.valid(),
          "functions unaffected"
    );

    near(
        reservedOverride.evaluate(),
         3,
         "functions result"
    );

    Expression missing("a+b", scope);

    check(
        missing.valid(),
          "missing binding still compiles"
    );

    check(bindings.erase("a"), "erase binding");

    check(
        std::isnan(
            missing.evaluate(
                0, 0, AngleMode::Radians, bindings
            )
        ),
        "missing value gives nan"
    );

    check(bindings.set("a", 2), "restore binding");

    near(
        missing.evaluate(
            0, 0, AngleMode::Radians, bindings
        ),
        0,
        "restore binding re-eval"
    );

    VariableScope isolated;

    check(
        isolated.declare("isolated"),
          "other scope declare"
    );

    check(
        !Expression("a+1", isolated).valid(),
          "other scope cannot see a"
    );

    check(
        !Expression("isolated+1", scope).valid(),
          "original scope cannot see isolated"
    );

    check(
        !Expression("unspecified+1", scope).valid(),
          "no implicit declarations"
    );

    check(
        scope.declare("later"),
          "extend scope"
    );

    check(
        !Expression("later+1").valid(),
          "old constructor unchanged after scope changes"
    );

    check(
        Expression("later+1", scope).valid(),
          "new expression sees later declaration"
    );

    Expression moved("a + b", scope);
    Expression destination(std::move(moved));

    check(
        destination.valid() && !moved.valid(),
          "move expression"
    );

    check(
        moved.referencedVariables().empty(),
          "moved-from safe references"
    );

    near(
        destination.evaluate(
            0, 0, AngleMode::Radians, bindings
        ),
         0,
         "moved expression eval"
    );

    // Hundreds of repeated evaluations must not mutate parser symbols
    // or leak old binding values into later calls.
    for (int i = 0; i < 1024; ++i) {
        VariableBindings local;

        check(
            local.set("a", static_cast<double>(i)),
              "bind in sweep"
        );

        check(
            local.set("b", 1),
              "bind b in sweep"
        );

        near(
            parabola.evaluate(
                2, 0, AngleMode::Radians, local
            ),
             i * 4.0 + 1,
             "repeated evaluation stability"
        );
    }

    std::cout
    << (fails == 0 ? "PASS: " : "FAIL: ")
    << checks
    << " NamedVariable assertions"
    << std::endl;

    return fails ? 1 : 0;
}
