#pragma once

#include "GraphViewport.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace rationalstudio {

// Screen-space drawing vertex for an explicit Cartesian y = f(x) curve.
// A moveTo vertex begins a new stroke; other vertices extend that stroke.
// The sampler has no dependency on Qt or on the expression parser.
struct CurveVertex {
    double screenX = 0.0;
    double screenY = 0.0;
    bool moveTo = false;
};

struct CurveSamplingOptions {
    double coarseStepPixels = 24.0;
    double tolerancePixels = 0.75;
    double minStepPixels = 0.375;
    unsigned maxDepth = 9;

    // Extra, bounded subdivision for finite terminal chords whose
    // approximation error remains high. This recovers steep continuous
    // curves without broadly relaxing discontinuity checks. Setting this
    // to zero retains the former terminal rejection behavior.
    unsigned maxTerminalRefinement = 8;

    // Hard maximum number of callback invocations for this curve.
    std::size_t maxEvaluations = 65536;

    // Optional fair coverage mode for interactive rendering. Divide the
    // screen into strips and share the curve budget across them. Expensive
    // poles near the left edge must not starve the right side of the graph.
    // The default preserves legacy sampling for existing API consumers.
    bool distributeEvaluationsAcrossViewport = false;
};

// Explicit, deterministic shared callback budget. A GraphCanvas paint can
// pass the same instance to multiple sampling calls. This limits how many
// mathematical callbacks execute, not their individual running time.
class CurveEvaluationBudget {
public:
    explicit CurveEvaluationBudget(std::size_t maximum) noexcept
        : maximum_(maximum) {}

    [[nodiscard]] std::size_t maximum() const noexcept { return maximum_; }
    [[nodiscard]] std::size_t consumed() const noexcept { return consumed_; }
    [[nodiscard]] std::size_t remaining() const noexcept { return maximum_ - consumed_; }
    [[nodiscard]] bool exhausted() const noexcept { return consumed_ == maximum_; }

    // For use by the sampler before invoking the user callback.
    [[nodiscard]] bool tryConsume() noexcept {
        if (exhausted()) return false;
        ++consumed_;
        return true;
    }

private:
    std::size_t maximum_;
    std::size_t consumed_ = 0;
};

enum class CurveTruncationReason {
    None,
    PerCurveLimit,
    SharedBudget,
    SpatialQuota
};

struct CurveSamples {
    std::vector<CurveVertex> vertices;
    std::size_t evaluations = 0;
    bool truncated = false;
    CurveTruncationReason truncationReason = CurveTruncationReason::None;
};

// Incremental visible-geometry build. Each advance has a fresh hard callback
// allowance; unfinished work is retained rather than cached as a final path.
// Completed tiles are never sampled again. The callback and viewport are owned
// by the build, so changing inputs requires creating a new build.
class CurveSamplingJob {
public:
    CurveSamplingJob(GraphViewport viewport, std::function<double(double)> function);
    ~CurveSamplingJob();
    CurveSamplingJob(const CurveSamplingJob&) = delete;
    CurveSamplingJob& operator=(const CurveSamplingJob&) = delete;
    bool advance(std::size_t maxEvaluations, CurveEvaluationBudget& sharedBudget);
    [[nodiscard]] const CurveSamples& samples() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Adaptively approximate y=f(x) over the current screen-width interval.
// The callback receives world-space x; the result is screen-space geometry.
// Assumes neither continuity nor mathematical domain knowledge. Detection
// of all discontinuities and high-frequency detail is not guaranteed.
[[nodiscard]] CurveSamples sampleExplicitCurve(
    const GraphViewport& viewport,
    const std::function<double(double)>& function,
    CurveSamplingOptions options = {},
    CurveEvaluationBudget* sharedBudget = nullptr
);

} // namespace rationalstudio
