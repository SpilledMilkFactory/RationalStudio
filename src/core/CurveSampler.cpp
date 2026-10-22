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
            AdaptiveSampler(
                const GraphViewport& viewport,
                const std::function<double(double)>& function,
                            const CurveSamplingOptions& options
            )
            : viewport_(viewport),
            function_(function),
            options_(options) {
            }

            CurveSamples run() {
                const double width =
                static_cast<double>(viewport_.width());

                Probe left = probe(0.0);

                for (
                    double nextX = options_.coarseStepPixels;
                nextX < width && !result_.truncated;
                nextX += options_.coarseStepPixels
                ) {
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
                if (
                    result_.evaluations >=
                    options_.maxEvaluations
                ) {
                    result_.truncated = true;

                    return {screenX, 0.0, false};
                }

                ++result_.evaluations;

                double worldY =
                std::numeric_limits<double>::quiet_NaN();

                try {
                    worldY = function_(
                        viewport_.screenToWorldX(screenX)
                    );
                } catch (...) {
                    // Domain errors thrown by a callback
                    // have the same effect as NaN.
                }

                // Do not send non-finite or out-of-range
                // coordinates to QPainter.
                //
                // The sample is treated as unrenderable
                // even if the mathematical function
                // returns a finite value of extreme magnitude.

                const double screenY =
                viewport_.worldToScreenY(worldY);

                return {
                    screenX,
                    screenY,
                    std::isfinite(worldY) &&
                    std::isfinite(screenY) &&
                    std::abs(screenY) <=
                    kMaxDrawingCoordinate
                };
            }

            void disconnect() noexcept {
                connected_ = false;
            }

            void append(
                const Probe& a,
                const Probe& b
            ) {
                if (!a.drawable || !b.drawable) {
                    disconnect();
                    return;
                }

                // Retain the baseline large-jump safeguard.
                //
                // Improved asymptote classification and
                // steep-line preservation are issues
                // #17 and #18.

                if (
                    std::abs(b.y - a.y) >
                    viewport_.height() * 0.7
                ) {
                    disconnect();
                    return;
                }

                if (
                    !connected_ ||
                    a.x != lastX_ ||
                    a.y != lastY_
                ) {
                    result_.vertices.push_back({
                        a.x,
                        a.y,
                        true
                    });
                }

                result_.vertices.push_back({
                    b.x,
                    b.y,
                    false
                });

                lastX_ = b.x;
                lastY_ = b.y;

                connected_ = true;
            }

            static double deviation(
                const Probe& a,
                const Probe& b,
                const Probe& interior
            ) noexcept {
                const double expected = std::lerp(
                    a.y,
                    b.y,
                    (interior.x - a.x) / (b.x - a.x)
                );

                return std::abs(
                    interior.y - expected
                );
            }

            // Final intervals must not bridge a sampled
            // undefined subinterval.
            //
            // Test two interior locations even at the
            // refinement limit.
            //
            // If either location is invalid, deliberately
            // omit the segment and break the path.
            //
            // This can leave a sub-pixel gap near a boundary.
            // It does not attempt to infer a domain endpoint
            // from finite samples.

            void appendTerminalHalf(
                const Probe& a,
                const Probe& b
            ) {
                if (
                    !a.drawable ||
                    !b.drawable ||
                    !(b.x > a.x)
                ) {
                    disconnect();
                    return;
                }

                const double span =
                b.x - a.x;

                const Probe q1 = probe(
                    a.x + span * 0.25
                );

                if (result_.truncated) {
                    return;
                }

                const Probe q3 = probe(
                    a.x + span * 0.75
                );

                if (result_.truncated) {
                    return;
                }

                if (!q1.drawable || !q3.drawable) {
                    disconnect();
                    return;
                }

                append(a, b);
            }

            void refine(
                const Probe& a,
                const Probe& b,
                unsigned depth
            ) {
                if (result_.truncated) {
                    return;
                }

                const double span =
                b.x - a.x;

                if (!(span > 0.0)) {
                    return;
                }

                const Probe mid = probe(
                    a.x + span * 0.5
                );

                if (result_.truncated) {
                    return;
                }

                const bool allDrawable =
                a.drawable &&
                mid.drawable &&
                b.drawable;

                const bool canSplit =
                depth < options_.maxDepth &&
                span > options_.minStepPixels;

                if (allDrawable) {
                    bool accurate =
                    deviation(a, b, mid) <=
                    options_.tolerancePixels;

                    // Check quarter points even for
                    // short accepted chords.
                    //
                    // The M1-02 optimization skipped
                    // these below six pixels, potentially
                    // bridging a narrow undefined pocket.
                    //
                    // Sample checks remain an approximation,
                    // not a mathematical proof of continuity.

                    if (accurate) {
                        const Probe q1 = probe(
                            a.x + span * 0.25
                        );

                        if (result_.truncated) {
                            return;
                        }

                        const Probe q3 = probe(
                            a.x + span * 0.75
                        );

                        if (result_.truncated) {
                            return;
                        }

                        accurate =
                        q1.drawable &&
                        q3.drawable &&
                        deviation(a, b, q1) <=
                        options_.tolerancePixels &&
                        deviation(a, b, q3) <=
                        options_.tolerancePixels;
                    }

                    if (accurate) {
                        append(a, b);
                        return;
                    }
                }

                if (canSplit) {
                    refine(
                        a,
                        mid,
                        depth + 1
                    );

                    refine(
                        mid,
                        b,
                        depth + 1
                    );
                } else {
                    // At the refinement limit, do not
                    // assume that finite endpoints imply
                    // a valid interior.
                    //
                    // Each drawable half gets its own
                    // interior domain checks before
                    // a segment is emitted.

                    appendTerminalHalf(a, mid);

                    if (result_.truncated) {
                        return;
                    }

                    appendTerminalHalf(mid, b);
                }
            }

            const GraphViewport& viewport_;

            const std::function<double(double)>& function_;

            const CurveSamplingOptions& options_;

            CurveSamples result_;

            bool connected_ = false;

            double lastX_ = 0.0;
            double lastY_ = 0.0;
        };

    } // namespace

    CurveSamples sampleExplicitCurve(
        const GraphViewport& viewport,
        const std::function<double(double)>& function,
                                     CurveSamplingOptions options
    ) {
        if (
            !viewport.isValid() ||
            !function ||
            !std::isfinite(options.coarseStepPixels) ||
            !std::isfinite(options.minStepPixels) ||
            !std::isfinite(options.tolerancePixels) ||
            options.coarseStepPixels <= 0.0 ||
            options.coarseStepPixels > 256.0 ||
            options.minStepPixels < 0.125 ||
            options.minStepPixels >
            options.coarseStepPixels ||
            options.tolerancePixels <= 0.0 ||
            options.maxDepth > 16 ||
            options.maxEvaluations < 4
        ) {
            return {};
        }

        return AdaptiveSampler(
            viewport,
            function,
            options
        ).run();
    }

} // namespace rationalstudio
