#include "CurveSampler.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace rationalstudio {
namespace {

// Protect QPainter from unreasonably large screen-space coordinates.
constexpr double kMaxDrawingCoordinate = 1e7;

struct Probe {
    double x = 0.0;
    double y = 0.0;
    bool drawable = false;
};

class AdaptiveSampler {
public:
    AdaptiveSampler(const GraphViewport& viewport,
                    const std::function<double(double)>& function,
                    const CurveSamplingOptions& options,
                    CurveEvaluationBudget* sharedBudget)
        : viewport_(viewport), function_(function), options_(options),
          sharedBudget_(sharedBudget) {}

    CurveSamples run() {
        const double width = static_cast<double>(viewport_.width());
        Probe left = probe(0.0);

        for (double nextX = options_.coarseStepPixels;
             nextX < width && !result_.truncated;
             nextX += options_.coarseStepPixels) {
            const Probe right = probe(nextX);
            refine(left, right, 0);
            left = right;
        }

        if (!result_.truncated && left.x < width) {
            refine(left, probe(width), 0);
        }
        return std::move(result_);
    }

private:
    Probe probe(double screenX) {
        if (result_.evaluations >= options_.maxEvaluations) {
            result_.truncated = true;
            result_.truncationReason = CurveTruncationReason::PerCurveLimit;
            return {screenX, 0.0, false};
        }
        if (sharedBudget_ && !sharedBudget_->tryConsume()) {
            result_.truncated = true;
            result_.truncationReason = CurveTruncationReason::SharedBudget;
            return {screenX, 0.0, false};
        }
        ++result_.evaluations;

        double worldY = std::numeric_limits<double>::quiet_NaN();
        try {
            worldY = function_(viewport_.screenToWorldX(screenX));
        } catch (...) {
            // Domain errors thrown by a callback have the same effect as NaN.
        }

        // Do not send non-finite or out-of-range coordinates to QPainter.
        // The sample is treated as unrenderable even if the mathematical
        // function returns a finite value of extreme magnitude.
        const double screenY = viewport_.worldToScreenY(worldY);
        return {screenX, screenY,
                std::isfinite(worldY) && std::isfinite(screenY) &&
                    std::abs(screenY) <= kMaxDrawingCoordinate};
    }

    void disconnect() noexcept { connected_ = false; }

    void append(const Probe& a, const Probe& b) {
        if (!a.drawable || !b.drawable) {
            disconnect();
            return;
        }

        // A large vertical change is not, by itself, a discontinuity:
        // a steep straight line may be perfectly continuous. The caller
        // verifies the chord with interior probes before append().

        if (!connected_ || a.x != lastX_ || a.y != lastY_) {
            result_.vertices.push_back({a.x, a.y, true});
        }
        result_.vertices.push_back({b.x, b.y, false});
        lastX_ = b.x;
        lastY_ = b.y;
        connected_ = true;
    }

    static double deviation(const Probe& a, const Probe& b,
                            const Probe& interior) noexcept {
        const double expected = std::lerp(
            a.y, b.y, (interior.x - a.x) / (b.x - a.x));
        return std::abs(interior.y - expected);
    }

    // At the normal recursion limit, a large approximation residual
    // cannot by itself distinguish high curvature from a discontinuity.
    // Probe the terminal chord and, when all probes remain drawable,
    // subdivide a few more times rather than immediately dropping it.
    //
    // This refinement has an explicit depth cap and participates in the
    // existing per-call evaluation budget. It is deliberately not a
    // promise of formal continuity classification.
    void refineTerminal(const Probe& a, const Probe& b,
                        unsigned recoveryDepth) {
        if (result_.truncated) return;
        if (!a.drawable || !b.drawable || !(b.x > a.x)) {
            disconnect();
            return;
        }

        const double span = b.x - a.x;
        const Probe q1 = probe(a.x + span * 0.25);
        if (result_.truncated) return;
        const Probe mid = probe(a.x + span * 0.5);
        if (result_.truncated) return;
        const Probe q3 = probe(a.x + span * 0.75);
        if (result_.truncated) return;

        if (!q1.drawable || !mid.drawable || !q3.drawable) {
            disconnect();
            return;
        }

        const double error = std::max({
            deviation(a, b, q1),
            deviation(a, b, mid),
            deviation(a, b, q3)
        });
        const double terminalTolerance =
            std::max(2.0, options_.tolerancePixels * 2.0);

        if (std::isfinite(error) && error <= terminalTolerance) {
            append(a, b);
            return;
        }

        // Avoid subdividing curve portions that are entirely offscreen.
        // Terminal rescue is valuable only if some sampled portion could
        // contribute to the visible graph. The original terminal check
        // already rejected this unresolved chord.
        const double guard = static_cast<double>(viewport_.height());
        const auto nearVisible = [guard](const Probe& p) {
            return p.y >= -guard && p.y <= 2.0 * guard;
        };
        const bool straddlesViewport =
            (a.y <= 0.0 && b.y >= viewport_.height()) ||
            (b.y <= 0.0 && a.y >= viewport_.height());
        const bool relevant = straddlesViewport || nearVisible(a) ||
                              nearVisible(q1) || nearVisible(mid) ||
                              nearVisible(q3) || nearVisible(b);

        // Floating-point screen coordinates must remain distinguishable.
        // 1/4096 pixel is also a practical numerical floor for this local
        // recovery pass, separate from the user's main sampling step.
        constexpr double kMinRecoverySpan = 1.0 / 4096.0;
        const double centerX = a.x + span * 0.5;
        if (relevant && recoveryDepth < options_.maxTerminalRefinement &&
            span > kMinRecoverySpan &&
            centerX > a.x && centerX < b.x) {
            refineTerminal(a, mid, recoveryDepth + 1);
            if (result_.truncated) return;
            refineTerminal(mid, b, recoveryDepth + 1);
        } else {
            // No reliable chord has been established. Keep a gap rather
            // than reconnecting a possible jump or asymptote.
            disconnect();
        }
    }

    void refine(const Probe& a, const Probe& b, unsigned depth) {
        if (result_.truncated) return;
        const double span = b.x - a.x;
        if (!(span > 0.0)) return;

        const Probe mid = probe(a.x + span * 0.5);
        if (result_.truncated) return;

        const bool allDrawable = a.drawable && mid.drawable && b.drawable;
        const bool canSplit = depth < options_.maxDepth &&
                              span > options_.minStepPixels;

        if (allDrawable) {
            bool accurate = deviation(a, b, mid) <= options_.tolerancePixels;

            // Check quarter points even for short accepted chords. The M1-02
            // optimization skipped these below six pixels, potentially
            // bridging a narrow undefined pocket. Sample checks remain an
            // approximation, not a mathematical proof of continuity.
            if (accurate) {
                const Probe q1 = probe(a.x + span * 0.25);
                if (result_.truncated) return;
                const Probe q3 = probe(a.x + span * 0.75);
                if (result_.truncated) return;
                accurate = q1.drawable && q3.drawable &&
                    deviation(a, b, q1) <= options_.tolerancePixels &&
                    deviation(a, b, q3) <= options_.tolerancePixels;
            }

            if (accurate) {
                append(a, b);
                return;
            }
        }

        if (canSplit) {
            refine(a, mid, depth + 1);
            refine(mid, b, depth + 1);
        } else {
            // At the refinement limit, do not assume that finite endpoints
            // imply a valid interior. Each drawable half gets its own
            // interior domain checks before a segment is emitted.
            refineTerminal(a, mid, 0);
            if (result_.truncated) return;
            refineTerminal(mid, b, 0);
        }
    }

    const GraphViewport& viewport_;
    const std::function<double(double)>& function_;
    const CurveSamplingOptions& options_;
    CurveEvaluationBudget* sharedBudget_;
    CurveSamples result_;
    bool connected_ = false;
    double lastX_ = 0.0;
    double lastY_ = 0.0;
};

} // namespace

CurveSamples sampleExplicitCurve(
    const GraphViewport& viewport,
    const std::function<double(double)>& function,
    CurveSamplingOptions options,
    CurveEvaluationBudget* sharedBudget) {

    if (!viewport.isValid() || !function ||
        !std::isfinite(options.coarseStepPixels) ||
        !std::isfinite(options.minStepPixels) ||
        !std::isfinite(options.tolerancePixels) ||
        options.coarseStepPixels <= 0.0 ||
        options.coarseStepPixels > 256.0 ||
        options.minStepPixels < 0.125 ||
        options.minStepPixels > options.coarseStepPixels ||
        options.tolerancePixels <= 0.0 ||
        options.maxDepth > 16 ||
        options.maxTerminalRefinement > 16 ||
        options.maxEvaluations < 4) {
        return {};
    }

    return AdaptiveSampler(viewport, function, options, sharedBudget).run();
}

} // namespace rationalstudio
