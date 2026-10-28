#include "ParametricSampler.hpp"
#include "Expression.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

using namespace rationalstudio;
namespace {
int checks = 0, failures = 0;
void check(bool yes, const char* what) {
    ++checks;
    if (!yes) { ++failures; std::cerr << "FAIL: " << what << '\n'; }
}
constexpr double pi = 3.14159265358979323846;
}
int main() {
    GraphViewport view;
    check(view.setDimensions(800, 600), "viewport dimensions");
    check(view.setView(0, 0, 50), "viewport scale");
    check(validParameterInterval(0, 2*pi), "valid interval");
    check(!validParameterInterval(1, 1), "zero interval rejected");
    check(!validParameterInterval(2, 1), "reversed interval rejected");
    check(!validParameterInterval(-1e99, 1), "out of range interval rejected");
    check(!validParameterInterval(0, std::numeric_limits<double>::infinity()), "nonfinite bound rejected");
    VariableScope scope;
    check(scope.declare("t"), "parameter scope");
    Expression x("cos(t)", scope), y("sin(t)", scope);
    check(x.valid() && y.valid(), "parameter expressions parse");
    const auto fn = [&](double t) {
        VariableBindings bindings;
        (void)bindings.set("t", t);
        return ParametricPoint{x.evaluate(0,0,AngleMode::Radians,bindings),
                               y.evaluate(0,0,AngleMode::Radians,bindings)};
    };
    const auto circle = sampleParametricCurve(view, fn, 0, 2*pi);
    check(!circle.truncated, "circle sampling completes");
    check(circle.evaluations >= 129, "circle samples representative positions");
    check(circle.vertices.size() > 80, "circle covers full interval");
    check(!circle.vertices.empty() && circle.vertices.front().moveTo, "starts stroke");
    if (!circle.vertices.empty()) {
        double minX=1e9, maxX=-1e9, minY=1e9, maxY=-1e9;
        for (const auto& v : circle.vertices) {
            check(std::isfinite(v.screenX) && std::isfinite(v.screenY), "finite circle vertex");
            minX=std::min(minX,v.screenX);maxX=std::max(maxX,v.screenX);
            minY=std::min(minY,v.screenY);maxY=std::max(maxY,v.screenY);
        }
        check(std::abs(minX-350)<2 && std::abs(maxX-450)<2, "circle x extrema");
        check(std::abs(minY-250)<2 && std::abs(maxY-350)<2, "circle y extrema");
        check(std::hypot(circle.vertices.back().screenX-circle.vertices.front().screenX,
                         circle.vertices.back().screenY-circle.vertices.front().screenY)<2,
              "circle endpoints close");
    }
    // Budget and incremental work both use the same evaluation token contract.
    ParametricSamplingJob incremental(view, fn, 0, 2*pi);
    std::size_t total = 0;
    bool complete = false;
    for (int paint=0; paint<3000 && !complete; ++paint) {
        CurveEvaluationBudget perPaint(31);
        complete = incremental.advance(31, perPaint);
        check(perPaint.consumed() <= 31, "per-paint hard budget");
        total += perPaint.consumed();
    }
    check(complete, "incremental circle completes");
    check(total == incremental.samples().evaluations, "no duplicate evaluations on retry");
    check(incremental.samples().vertices.size() == circle.vertices.size(), "incremental geometry stable");
    CurveEvaluationBudget shared(16);
    const auto truncated = sampleParametricCurve(view, fn, 0, 2*pi, 1000, &shared);
    check(truncated.truncated && shared.consumed() <= 16, "shared budget respected");
    // Invalid samples must not be drawn as finite points or bridged in one stroke.
    const auto broken = sampleParametricCurve(view, [](double t) -> ParametricPoint {
        if (t>0.45 && t<0.55) return {NAN,NAN};
        return {t, t*t};
    }, 0, 1);
    check(!broken.truncated, "invalid domain sampling terminates");
    int starts = 0;
    for (const auto& v : broken.vertices) {
        if (v.moveTo) ++starts;
        check(std::isfinite(v.screenX) && std::isfinite(v.screenY), "no invalid screen vertices");
    }
    check(starts >= 2, "invalid interval creates disconnected branches");
    // Empty function is treated as completed empty geometry.
    const auto empty = sampleParametricCurve(view, {}, 0, 1);
    check(empty.vertices.empty() && !empty.truncated, "empty callback safe");
    std::cout << (failures ? "FAIL: " : "PASS: ") << checks << " ParametricSampler assertions\n";
    return failures ? 1 : 0;
}
