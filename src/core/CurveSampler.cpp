#include "CurveSampler.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace rationalstudio {

    namespace {

        // Match the M0 renderer's protection against extreme painter coordinates.
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
                    // Treat user-supplied evaluation failures
                    // as undefined points.
                }

                const double screenY =
                viewport_.worldToScreenY(worldY);

                return {
                    screenX,
                    screenY,
                    std::isfinite(screenY) &&
                    std::abs(screenY) <= kMaxDrawingCoordinate
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

                // Preserve the M0 large-jump break as a
                // conservative safeguard.
                //
                // Thorough discontinuity classification
                // belongs to issues #16-17.
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
                const Probe& middle
            ) noexcept {
                const double expected = std::lerp(
                    a.y,
                    b.y,
                    (middle.x - a.x) / (b.x - a.x)
                );

                return std::abs(middle.y - expected);
            }

            void refine(
                const Probe& a,
                const Probe& b,
                unsigned depth
            ) {
                if (result_.truncated) {
                    return;
                }

                const double span = b.x - a.x;

                if (span <= 0.0) {
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

                    // Quarter probes catch some symmetric
                    // or inflected features that look straight
                    // when only the endpoints and midpoint
                    // are used.
                    if (accurate && span > 6.0) {
                        const Probe q1 = probe(
                            a.x + span * 0.25
                        );

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
                    // At the refinement limit,
                    // retain only drawable halves.
                    //
                    // A later reliability issue will refine
                    // pole/domain boundary treatment.
                    append(a, mid);
                    append(mid, b);
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
            options.minStepPixels > options.coarseStepPixels ||
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
