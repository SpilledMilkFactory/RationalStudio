#include "CurveSampler.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <utility>

namespace rationalstudio {
namespace {

// Protect QPainter from unreasonably large screen-space coordinates.
constexpr double kMaxDrawingCoordinate = 1e7;

struct SamplingYield {};

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

    bool advance(std::size_t allowance, CurveEvaluationBudget& shared) {
        if (!viewport_.isValid() || !function_) return true;
        incremental_ = true;
        sharedBudget_ = &shared;
        advanceEnd_ = result_.evaluations + allowance;
        const double width = viewport_.width();
        while (nextTile_ < width) {
            const auto savedSize = result_.vertices.size();
            const bool savedConnected = connected_;
            const double savedX = lastX_, savedY = lastY_;
            const double end = std::min(width, nextTile_ + options_.coarseStepPixels);
            try {
                sampleRange(nextTile_, end);
            } catch (const SamplingYield&) {
                // Replay only this unfinished tile on the next advance. Its
                // probes are memoized, and none of its partial strokes escape.
                result_.vertices.resize(savedSize);
                connected_ = savedConnected;
                lastX_ = savedX;
                lastY_ = savedY;
                return false;
            }
            nextTile_ = end;
            retryProbes_.clear();
        }
        return true;
    }

    const CurveSamples& samples() const { return result_; }

    CurveSamples run() {
        const double width = static_cast<double>(viewport_.width());
        if (!options_.distributeEvaluationsAcrossViewport) {
            sampleRange(0.0, width);
            return std::move(result_);
        }

        // At least 128 evaluations per strip for ordinary configurations.
        // Small configured budgets use fewer strips rather than producing
        // dozens of empty regions with only a handful of probes each.
        const std::size_t strips = std::max<std::size_t>(1,
            std::min<std::size_t>(32, options_.maxEvaluations / 128));
        bool anyStripTruncated = false;
        for (std::size_t i = 0; i < strips && !result_.truncated; ++i) {
            const std::size_t remaining =
                options_.maxEvaluations - result_.evaluations;
            if (remaining < 4) {
                result_.truncated = true;
                result_.truncationReason = CurveTruncationReason::PerCurveLimit;
                break;
            }
            const std::size_t remainingStrips = strips - i;
            const std::size_t quota = std::max<std::size_t>(4,
                remaining / remainingStrips);
            stripEvaluationEnd_ = result_.evaluations +
                std::min(quota, remaining);
            stripExhausted_ = false;

            // Each strip starts a new stroke. This forbids lines bridging
            // unknown/discontinuous regions at strip boundaries.
            disconnect();
            const double begin = width * static_cast<double>(i) /
                                 static_cast<double>(strips);
            const double end = width * static_cast<double>(i + 1) /
                               static_cast<double>(strips);
            sampleRange(begin, end);
            anyStripTruncated = anyStripTruncated || stripExhausted_;
        }

        if (anyStripTruncated && !result_.truncated) {
            result_.truncated = true;
            result_.truncationReason = CurveTruncationReason::SpatialQuota;
        }
        return std::move(result_);
    }

    void sampleRange(double begin, double end) {
        Probe left = probe(begin);
        for (double nextX = begin + options_.coarseStepPixels;
             nextX < end && !stopped();
             nextX += options_.coarseStepPixels) {
            const Probe right = probe(nextX);
            if (stopped()) break;
            refine(left, right, 0);
            left = right;
        }
        if (!stopped() && left.x < end) {
            const Probe right = probe(end);
            if (!stopped()) refine(left, right, 0);
        }
    }

    [[nodiscard]] bool stopped() const noexcept {
        return result_.truncated || stripExhausted_;
    }

private:
    Probe probe(double screenX) {
        if (incremental_) {
            const auto found = retryProbes_.find(screenX);
            if (found != retryProbes_.end()) return found->second;
            if (result_.evaluations >= advanceEnd_ || sharedBudget_->exhausted())
                throw SamplingYield{};
        }
        if (options_.distributeEvaluationsAcrossViewport &&
            result_.evaluations >= stripEvaluationEnd_) {
            stripExhausted_ = true;
            return {screenX, 0.0, false};
        }
        if (!incremental_ && result_.evaluations >= options_.maxEvaluations) {
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
        const Probe value{screenX, screenY,
                std::isfinite(worldY) && std::isfinite(screenY) &&
                    std::abs(screenY) <= kMaxDrawingCoordinate};
        if (incremental_) retryProbes_.emplace(screenX, value);
        return value;
    }

    void disconnect() noexcept { connected_ = false; }

    void append(const Probe& first, const Probe& second) {
        Probe a = first, b = second;
        if (incremental_ && a.drawable && b.drawable) {
            const double low = -2.0, high = viewport_.height() + 2.0;
            if (std::max(a.y, b.y) < low || std::min(a.y, b.y) > high) {
                disconnect();
                return;
            }
            const auto clipped = [&](const Probe& point) {
                const double y = std::clamp(point.y, low, high);
                if (y == point.y || first.y == second.y) return point;
                const double t = (y - first.y) / (second.y - first.y);
                return Probe{std::lerp(first.x, second.x, t), y, true};
            };
            a = clipped(a);
            b = clipped(b);
        }
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
    bool acceptVisibleChord(const Probe& a, const Probe& b, const Probe& q1,
                            const Probe& mid, const Probe& q3) {
        const double span = b.x - a.x;
        if (q1.drawable && q3.drawable) {
            const double low = std::min({a.y, q1.y, mid.y, q3.y, b.y});
            const double high = std::max({a.y, q1.y, mid.y, q3.y, b.y});
            const bool monotone =
                (a.y <= q1.y && q1.y <= mid.y && mid.y <= q3.y && q3.y <= b.y) ||
                (a.y >= q1.y && q1.y >= mid.y && mid.y >= q3.y && q3.y >= b.y);
            if (monotone && (high < -2.0 || low > viewport_.height() + 2.0)) {
                disconnect();
                return true;
            }
            // Measure geometric error for steep smooth chords. All four
            // local slopes must agree in direction and magnitude, which
            // rejects poles and finite jumps before using this metric.
            const double d[] = {q1.y-a.y, mid.y-q1.y, q3.y-mid.y, b.y-q3.y};
            const auto [minimum, maximum] = std::minmax_element(std::begin(d), std::end(d));
            const double smallest = std::min(std::abs(*minimum), std::abs(*maximum));
            const double largest = std::max(std::abs(*minimum), std::abs(*maximum));
            if ((*minimum > 0.0 || *maximum < 0.0) && largest <= 16.0 * smallest) {
                const double residual = std::max({deviation(a, b, q1), deviation(a, b, mid), deviation(a, b, q3)});
                const double normalizer = std::hypot(1.0, (b.y-a.y)/span);
                if (residual / normalizer <= options_.tolerancePixels) {
                    append(a, b);
                    return true;
                }
            }
        }
        return false;
    }

    void refineTerminal(const Probe& a, const Probe& b,
                        unsigned recoveryDepth) {
        if (stopped()) return;
        if (!(b.x > a.x)) return;
        if (!a.drawable || !b.drawable) {
            if (incremental_ && recoveryDepth < options_.maxTerminalRefinement) {
                const Probe mid = probe(std::midpoint(a.x, b.x));
                if (a.drawable || mid.drawable || b.drawable) {
                    refineTerminal(a, mid, recoveryDepth + 1);
                    refineTerminal(mid, b, recoveryDepth + 1);
                    return;
                }
            }
            disconnect();
            return;
        }

        const double span = b.x - a.x;
        const Probe q1 = probe(a.x + span * 0.25);
        if (stopped()) return;
        const Probe mid = probe(a.x + span * 0.5);
        if (stopped()) return;
        const Probe q3 = probe(a.x + span * 0.75);
        if (stopped()) return;

        if (!q1.drawable || !mid.drawable || !q3.drawable) {
            if (incremental_ && recoveryDepth < options_.maxTerminalRefinement) {
                refineTerminal(a, mid, recoveryDepth + 1);
                refineTerminal(mid, b, recoveryDepth + 1);
            } else {
                disconnect();
            }
            return;
        }

        if (incremental_ && acceptVisibleChord(a, b, q1, mid, q3)) return;

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
            span > (incremental_ ? 1e-12 : kMinRecoverySpan) &&
            centerX > a.x && centerX < b.x) {
            refineTerminal(a, mid, recoveryDepth + 1);
            if (stopped()) return;
            refineTerminal(mid, b, recoveryDepth + 1);
        } else {
            // No reliable chord has been established. Keep a gap rather
            // than reconnecting a possible jump or asymptote.
            disconnect();
        }
    }

    void refine(const Probe& a, const Probe& b, unsigned depth) {
        if (stopped()) return;
        const double span = b.x - a.x;
        if (!(span > 0.0)) return;

        const Probe mid = probe(a.x + span * 0.5);
        if (stopped()) return;

        const bool allDrawable = a.drawable && mid.drawable && b.drawable;
        if (incremental_ && !a.drawable && !mid.drawable && !b.drawable) {
            disconnect();
            return;
        }
        const bool canSplit = depth < options_.maxDepth &&
                              span > options_.minStepPixels;

        if (allDrawable && incremental_) {
            const Probe q1 = probe(a.x + span * 0.25);
            const Probe q3 = probe(a.x + span * 0.75);
            if (acceptVisibleChord(a, b, q1, mid, q3)) return;
        }

        if (allDrawable) {
            bool accurate = deviation(a, b, mid) <= options_.tolerancePixels;

            // Check quarter points even for short accepted chords. The M1-02
            // optimization skipped these below six pixels, potentially
            // bridging a narrow undefined pocket. Sample checks remain an
            // approximation, not a mathematical proof of continuity.
            if (accurate) {
                const Probe q1 = probe(a.x + span * 0.25);
                if (stopped()) return;
                const Probe q3 = probe(a.x + span * 0.75);
                if (stopped()) return;
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
            if (stopped()) return;
            refineTerminal(mid, b, 0);
        }
    }

    const GraphViewport& viewport_;
    const std::function<double(double)>& function_;
    const CurveSamplingOptions& options_;
    CurveEvaluationBudget* sharedBudget_;
    CurveSamples result_;
    bool incremental_ = false;
    double nextTile_ = 0.0;
    std::size_t advanceEnd_ = 0;
    std::map<double, Probe> retryProbes_;
    bool connected_ = false;
    std::size_t stripEvaluationEnd_ = 0;
    bool stripExhausted_ = false;
    double lastX_ = 0.0;
    double lastY_ = 0.0;
};

} // namespace

struct CurveSamplingJob::Impl {
    GraphViewport viewport;
    std::function<double(double)> function;
    CurveSamplingOptions options;
    AdaptiveSampler sampler;

    Impl(GraphViewport view, std::function<double(double)> callback)
        : viewport(view), function(std::move(callback)), options(makeOptions(view)),
          sampler(viewport, function, options, nullptr) {}

    static CurveSamplingOptions makeOptions(const GraphViewport& view) {
        CurveSamplingOptions options;
        // Small world intervals prevent repeated poles from aliasing away at
        // wide zooms. Subpixel horizontal spans still contain visible tails.
        options.coarseStepPixels = std::min(24.0, view.pixelsPerUnit() * 0.5);
        options.minStepPixels = std::min(0.375, options.coarseStepPixels / 64.0);
        options.maxTerminalRefinement = 16;
        return options;
    }
};

CurveSamplingJob::CurveSamplingJob(GraphViewport viewport,
                                 std::function<double(double)> function)
    : impl_(std::make_unique<Impl>(viewport, std::move(function))) {}
CurveSamplingJob::~CurveSamplingJob() = default;
bool CurveSamplingJob::advance(std::size_t maximum, CurveEvaluationBudget& shared) {
    return impl_->sampler.advance(maximum, shared);
}
const CurveSamples& CurveSamplingJob::samples() const { return impl_->sampler.samples(); }

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
