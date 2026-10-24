#include "CurveSampler.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <numbers>
#include <stdexcept>

using namespace rationalstudio;

namespace {
std::size_t assertions = 0;
void require(bool condition, const char* message) {
    ++assertions;
    if (!condition) throw std::runtime_error(message);
}

GraphViewport wideViewport() {
    GraphViewport viewport;
    require(viewport.setDimensions(900, 600), "900x600 viewport");
    require(viewport.setView(130.0, 0.0, 1.38), "wide finite view");
    return viewport;
}

std::size_t horizontalBins(const CurveSamples& samples, double width) {
    std::array<bool, 10> occupied{};
    for (const auto& vertex : samples.vertices) {
        require(std::isfinite(vertex.screenX) &&
                std::isfinite(vertex.screenY), "finite emitted vertices");
        require(vertex.screenX >= 0.0 && vertex.screenX <= width,
                "vertex within viewport width");
        const auto bin = std::min<std::size_t>(9,
            static_cast<std::size_t>(10.0 * vertex.screenX / width));
        occupied[bin] = true;
    }
    return static_cast<std::size_t>(std::count(occupied.begin(),
                                              occupied.end(), true));
}

void tangentAcrossWideViewport() {
    const auto viewport = wideViewport();
    CurveSamplingOptions options;
    options.maxEvaluations = 8192;

    // This control reproduces the reported failure: left-to-right
    // refinement consumes most evaluations around early tangent poles.
    const auto original = sampleExplicitCurve(viewport,
        [](double x) { return std::tan(x); }, options);
    require(original.truncated, "legacy tangent sampling exhausts budget");
    require(horizontalBins(original, 900.0) <= 4,
            "control reproduces left-side starvation");

    options.distributeEvaluationsAcrossViewport = true;
    std::size_t calls = 0;
    const auto distributed = sampleExplicitCurve(viewport,
        [&](double x) { ++calls; return std::tan(x); }, options);
    require(!distributed.vertices.empty(), "distributed tangent has geometry");
    require(distributed.evaluations == calls, "exact evaluation accounting");
    require(calls <= 8192, "hard per-curve callback ceiling preserved");
    require(distributed.truncated,
            "omitted/refinement-limited intervals are explicitly reported");
    require(distributed.truncationReason == CurveTruncationReason::SpatialQuota,
            "spatial quota reported rather than false completion");
    require(horizontalBins(distributed, 900.0) >= 9,
            "repeating tangent geometry distributed across visible width");
}

void finiteSharedBudgets() {
    const auto viewport = wideViewport();
    for (std::size_t maximum : {64u, 512u, 8192u}) {
        CurveEvaluationBudget shared(maximum);
        CurveSamplingOptions options;
        options.maxEvaluations = 8192;
        options.distributeEvaluationsAcrossViewport = true;
        std::size_t callbacks = 0;
        const auto samples = sampleExplicitCurve(viewport, [&](double x) {
            ++callbacks;
            return std::tan(x);
        }, options, &shared);
        require(shared.consumed() == callbacks,
                "shared budget counts every callback exactly once");
        require(samples.evaluations == callbacks,
                "sample count agrees with callback count");
        require(callbacks <= maximum, "shared budget never exceeded");
        require(!samples.vertices.empty() || maximum < 128,
                "nonempty geometry for adequate shared budget");
    }
}

void smoothAndRestrictedControls() {
    const auto viewport = wideViewport();
    CurveSamplingOptions options;
    options.maxEvaluations = 8192;
    options.distributeEvaluationsAcrossViewport = true;
    const auto line = sampleExplicitCurve(viewport, [](double x) {
        return x * 0.25;
    }, options);
    require(!line.truncated, "easy straight line fully sampled");
    require(horizontalBins(line, 900.0) == 10,
            "straight line continues across viewport");

    const auto root = sampleExplicitCurve(viewport, [](double x) {
        return std::sqrt(x);
    }, options);
    require(!root.vertices.empty(), "restricted-domain curve drawn");
    require(horizontalBins(root, 900.0) >= 2,
            "restricted-domain geometry not starved");
    for (const auto& vertex : root.vertices) {
        require(viewport.screenToWorldX(vertex.screenX) >= -1e-10,
                "sqrt segments do not originate in invalid x-domain");
    }
}
} // namespace

int main() {
    try {
        tangentAcrossWideViewport();
        finiteSharedBudgets();
        smoothAndRestrictedControls();
        std::cout << "PASS: " << assertions
                  << " CurveCoverageRegression assertions\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << "\n";
        return 1;
    }
}
