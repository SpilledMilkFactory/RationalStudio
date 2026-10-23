#include "GraphViewport.hpp"
#include "CurveSampler.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace rationalstudio;

namespace {
std::size_t assertions = 0;
void require(bool condition, const std::string& detail) {
    if (!condition) throw std::runtime_error(detail);
    ++assertions;
}
void near(double actual, double expected, double tolerance, const std::string& detail) {
    require(std::isfinite(actual) && std::isfinite(expected) &&
            std::abs(actual - expected) <= tolerance, detail);
}
GraphViewport viewport(int width, int height, double cx = 0, double cy = 0,
                       double scale = 58) {
    GraphViewport view;
    require(view.setDimensions(width, height), "valid viewport dimensions");
    require(view.setView(cx, cy, scale), "valid viewport center and scale");
    return view;
}
void sameState(const GraphViewport& left, const GraphViewport& right,
               const std::string& why) {
    require(left.width() == right.width() && left.height() == right.height() &&
            left.centerX() == right.centerX() && left.centerY() == right.centerY() &&
            left.pixelsPerUnit() == right.pixelsPerUnit(), why);
}
void transformsAndZoomAnchors() {
    constexpr std::array dimensions = {std::array{351, 301}, std::array{801, 601},
                                       std::array{1920, 1080}};
    for (const auto [width, height] : dimensions) {
        for (double scale : {0.5, 58.0, 500.0}) {
            for (double center : {-125.5, 0.0, 7.125}) {
                auto view = viewport(width, height, center, -0.25 * center, scale);
                for (double fraction : {0.0, 0.137, 0.5, 0.873, 1.0}) {
                    const double sx = fraction * width;
                    const double sy = (1.0 - fraction) * height;
                    near(view.worldToScreenX(view.screenToWorldX(sx)), sx,
                         1e-7, "X coordinate transform roundtrip");
                    near(view.worldToScreenY(view.screenToWorldY(sy)), sy,
                         1e-7, "Y coordinate transform roundtrip");
                }
                const double sx = 0.23 * width;
                const double sy = 0.81 * height;
                const double anchorX = view.screenToWorldX(sx);
                const double anchorY = view.screenToWorldY(sy);
                require(view.zoomAt(1.15, sx, sy), "off-center zoom accepted");
                near(view.screenToWorldX(sx), anchorX, 1e-8,
                     "cursor world X remains anchored after zoom");
                near(view.screenToWorldY(sy), anchorY, 1e-8,
                     "cursor world Y remains anchored after zoom");
                require(view.zoomAt(1.0 / 1.15, sx, sy), "reverse zoom accepted");
                near(view.pixelsPerUnit(), scale, 1e-8,
                     "roundtrip zoom restores scale");
            }
        }
    }
    auto view = viewport(801, 601, 1.25, -3.75, 100);
    const auto saved = view;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    require(!view.setView(nan, 0, 58), "NaN center rejected");
    require(!view.setView(0, inf, 58), "infinite center rejected");
    require(!view.setView(0, 0, nan), "NaN scale rejected");
    require(!view.setDimensions(0, 601), "zero width rejected");
    require(!view.panPixels(inf, 0), "infinite pan rejected");
    require(!view.zoomAt(0, 100, 100), "zero zoom rejected");
    require(!view.zoomAt(1.2, nan, 100), "NaN cursor rejected");
    sameState(view, saved, "invalid operations preserve viewport atomically");
    require(view.setView(1.25, -3.75, GraphViewport::kMaxWheelScale),
            "upper wheel-scale boundary accepted");
    const auto upper = view;
    for (int i = 0; i < 64; ++i)
        require(view.zoomAt(1.2, 300.5, 200.25), "upper zoom clamp accepted");
    sameState(view, upper, "upper clamp has no viewport drift");
    require(view.setView(1.25, -3.75, GraphViewport::kMinWheelScale),
            "lower wheel-scale boundary accepted");
    const auto lower = view;
    for (int i = 0; i < 64; ++i)
        require(view.zoomAt(0.8, 300.5, 200.25), "lower zoom clamp accepted");
    sameState(view, lower, "lower clamp has no viewport drift");
}

void geometryInvariant(const CurveSamples& samples, const GraphViewport& view,
                       const std::string& context) {
    require(samples.evaluations > 0, context + ": sampler evaluated callback");
    require(samples.evaluations <= 65536, context + ": within default maxEvaluations");
    for (const auto& vertex : samples.vertices) {
        require(std::isfinite(vertex.screenX) && std::isfinite(vertex.screenY),
                context + ": finite drawing vertex");
        require(vertex.screenX >= -1e-7 && vertex.screenX <= view.width() + 1e-7,
                context + ": vertex in sampled horizontal range");
        require(std::abs(vertex.screenY) <= 1e7,
                context + ": drawing coordinate magnitude bounded");
    }
}
void accuracyAndSampling() {
    for (const auto& view : {viewport(800, 600), viewport(801, 601, 1.1, -0.5, 80),
                             viewport(1200, 720, -2.25, 1.25, 45)}) {
        const std::array<std::pair<const char*, std::function<double(double)>>, 3> controls {{
            {"linear", [](double x) { return 2*x - 1; }},
            {"parabola", [](double x) { return 0.25*x*x - 0.5; }},
            {"sine", [](double x) { return std::sin(x); }}
        }};
        for (const auto& [label, fn] : controls) {
            CurveSamplingOptions options;
            options.tolerancePixels = 0.35;
            const auto sampled = sampleExplicitCurve(view, fn, options);
            require(!sampled.truncated, std::string(label) + ": ordinary curve complete");
            require(!sampled.vertices.empty(), std::string(label) + ": emits drawing geometry");
            geometryInvariant(sampled, view, label);
            // Geometric chord midpoint error checks actual emitted line segments,
            // rather than exact vertex counts or a rasterization implementation.
            std::size_t segments = 0;
            for (std::size_t i = 1; i < sampled.vertices.size(); ++i) {
                const auto& a = sampled.vertices[i-1];
                const auto& b = sampled.vertices[i];
                if (b.moveTo || a.screenX == b.screenX) continue;
                const double midpoint = 0.5 * (a.screenX + b.screenX);
                const double exact = view.worldToScreenY(fn(view.screenToWorldX(midpoint)));
                if (!std::isfinite(exact) || std::abs(exact) > 1e6) continue;
                const double approximate = std::lerp(a.screenY, b.screenY, 0.5);
                near(approximate, exact, 2.0,
                     std::string(label) + ": drawn chord follows mathematical function");
                ++segments;
            }
            require(segments >= 2, std::string(label) + ": multiple drawn segments");
        }
    }
}
void domainsAndDiscontinuities() {
    const auto view = viewport(1001, 701, 0.13, 0, 100);
    const auto sqrtCurve = sampleExplicitCurve(view, [](double x) {
        return x >= 0.0 ? std::sqrt(x) : std::numeric_limits<double>::quiet_NaN();
    });
    require(!sqrtCurve.vertices.empty(), "restricted domain retains valid branch");
    geometryInvariant(sqrtCurve, view, "sqrt domain");
    for (const auto& v : sqrtCurve.vertices) {
        require(view.screenToWorldX(v.screenX) >= -1e-8,
                "sqrt never draws negative domain vertices");
    }
    const auto holes = sampleExplicitCurve(view, [](double x) {
        if (x >= -0.2 && x <= 0.4) return std::numeric_limits<double>::quiet_NaN();
        return 0.5 * x;
    });
    geometryInvariant(holes, view, "invalid pocket");
    for (std::size_t i = 1; i < holes.vertices.size(); ++i) {
        if (holes.vertices[i].moveTo) continue;
        const double x1 = view.screenToWorldX(holes.vertices[i-1].screenX);
        const double x2 = view.screenToWorldX(holes.vertices[i].screenX);
        require(!(x1 < -0.2 && x2 > 0.4), "domain hole is not bridged");
    }
    const double pole = 0.133;
    const auto reciprocal = sampleExplicitCurve(view, [pole](double x) {
        return 1.0 / (x - pole);
    });
    geometryInvariant(reciprocal, view, "reciprocal pole");
    require(!reciprocal.vertices.empty(), "reciprocal has drawable branches");
    for (std::size_t i = 1; i < reciprocal.vertices.size(); ++i) {
        if (reciprocal.vertices[i].moveTo) continue;
        const double x1 = view.screenToWorldX(reciprocal.vertices[i-1].screenX);
        const double x2 = view.screenToWorldX(reciprocal.vertices[i].screenX);
        require(!(x1 < pole && x2 > pole),
                "reciprocal segment never bridges non-grid-aligned pole");
    }
    const auto step = sampleExplicitCurve(view, [](double x) { return std::floor(x); });
    geometryInvariant(step, view, "finite step");
    for (std::size_t i = 1; i < step.vertices.size(); ++i) {
        if (step.vertices[i].moveTo) continue;
        const double x1 = view.screenToWorldX(step.vertices[i-1].screenX);
        const double x2 = view.screenToWorldX(step.vertices[i].screenX);
        // Test a boundary strictly inside a segment, not coincident with
        // one of its endpoints (where an ordinary step legitimately ends).
        const double between = std::floor(x1) + 1.0;
        require(!(x1 < between && between < x2),
                "step segment never crosses an integer jump");
    }
    const auto throwing = sampleExplicitCurve(view, [](double x) -> double {
        if (x < 0.0) throw std::domain_error("not defined");
        return x;
    });
    geometryInvariant(throwing, view, "throwing domain callback");
}
void deterministicLimits() {
    const auto view = viewport(801, 601);
    const auto complicated = [](double x) { return std::sin(25*x) + std::cos(67*x); };
    for (std::size_t limit : {0u, 1u, 2u, 3u, 4u, 5u, 8u, 16u, 31u,
                              64u, 128u, 512u, 2048u}) {
        CurveEvaluationBudget budget(limit);
        CurveSamplingOptions options;
        options.maxEvaluations = 32768;
        std::size_t calls = 0;
        std::size_t reported = 0;
        for (int i = 0; i < 3; ++i) {
            const auto result = sampleExplicitCurve(view, [&](double x) {
                ++calls;
                return complicated(x);
            }, options, &budget);
            reported += result.evaluations;
            require(result.evaluations <= budget.maximum(),
                    "each curve evaluation count within shared maximum");
            require(result.truncationReason == CurveTruncationReason::None ||
                    result.truncated, "truncation reason agrees with flag");
        }
        require(calls == budget.consumed(), "shared budget equals callback count");
        require(reported == calls, "per-curve totals equal callback count");
        require(budget.consumed() <= limit, "no shared evaluation budget overshoot");
    }
    for (std::size_t limit : {4u, 8u, 16u, 64u, 256u}) {
        CurveSamplingOptions options;
        options.maxEvaluations = limit;
        std::size_t calls = 0;
        const auto result = sampleExplicitCurve(view, [&](double x) {
            ++calls;
            return complicated(x);
        }, options);
        require(calls == result.evaluations, "exact per-curve callback count");
        require(calls <= limit, "never exceed per-curve cap");
        require(result.truncated, "demanding curve is truncated under tight cap");
        require(result.truncationReason == CurveTruncationReason::PerCurveLimit,
                "per-curve truncation reason is explicit");
    }
    CurveSamplingOptions invalid;
    invalid.coarseStepPixels = 0;
    int calls = 0;
    const auto result = sampleExplicitCurve(view, [&](double x) {
        ++calls;
        return x;
    }, invalid);
    require(result.vertices.empty() && result.evaluations == 0 && calls == 0,
            "invalid sampling settings do not evaluate callbacks");
}
} // namespace

int main() {
    try {
        transformsAndZoomAnchors();
        accuracyAndSampling();
        domainsAndDiscontinuities();
        deterministicLimits();
        std::cout << "PASS: " << assertions << " GraphRegressionMatrix assertions\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return 1;
    }
}
