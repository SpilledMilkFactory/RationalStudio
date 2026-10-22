#include "CurveSampler.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

using rationalstudio::CurveSamples;
using rationalstudio::CurveSamplingOptions;
using rationalstudio::CurveVertex;
using rationalstudio::GraphViewport;
using rationalstudio::sampleExplicitCurve;

namespace {

    std::size_t checks = 0;

    void require(bool condition, const std::string& description) {
        if (!condition) throw std::runtime_error(description);
        ++checks;
    }

    GraphViewport makeViewport(double centerX = 0.0,
                               double centerY = 0.0,
                               double scale = 100.0) {
        GraphViewport viewport;
        require(viewport.setDimensions(800, 600), "viewport dimensions");
        require(viewport.setView(centerX, centerY, scale), "viewport state");
        return viewport;
                               }

                               // Inspect actual output paths, not merely the values evaluated by the
                               // callback. A moveTo breaks the path even when points are extremely close.
                               bool continuousAcross(const CurveSamples& samples, double screenX) {
                                   bool pathStarted = false;
                                   bool pathHasSegment = false;
                                   double startX = 0.0;
                                   double endX = 0.0;

                                   for (const CurveVertex& vertex : samples.vertices) {
                                       if (vertex.moveTo) {
                                           if (pathStarted && pathHasSegment &&
                                               startX < screenX && endX > screenX) return true;
                                           pathStarted = true;
                                           pathHasSegment = false;
                                           startX = endX = vertex.screenX;
                                       } else {
                                           pathHasSegment = true;
                                           endX = vertex.screenX;
                                       }
                                   }

                                   return pathStarted && pathHasSegment &&
                                   startX < screenX && endX > screenX;
                               }

                               // Every connected chord must closely approximate its mathematical function
                               // at 1/4, 1/2 and 3/4 of that chord. Check screen pixels, not world units.
                               template <class Function>
                               void checkChords(const CurveSamples& samples, const GraphViewport& viewport,
                                                const Function& f, double errorPixels) {
                                   CurveVertex previous{};
                                   bool started = false;
                                   for (const CurveVertex& vertex : samples.vertices) {
                                       require(std::isfinite(vertex.screenX) &&
                                       std::isfinite(vertex.screenY), "finite output geometry");
                                       require(vertex.screenX >= 0.0 &&
                                       vertex.screenX <= viewport.width(), "horizontal bounds");

                                       if (vertex.moveTo) {
                                           previous = vertex;
                                           started = true;
                                           continue;
                                       }
                                       require(started, "lineTo follows moveTo");
                                       require(vertex.screenX > previous.screenX, "increasing X segments");

                                       for (double fraction : {0.25, 0.50, 0.75}) {
                                           const double x = std::lerp(previous.screenX, vertex.screenX,
                                                                      fraction);
                                           const double worldX = viewport.screenToWorldX(x);
                                           const double actual = viewport.worldToScreenY(f(worldX));
                                           const double line = std::lerp(previous.screenY,
                                                                         vertex.screenY, fraction);
                                           require(std::isfinite(actual) &&
                                           std::abs(actual - line) <= errorPixels,
                                                   "steep curve chord approximates sampled function");
                                       }
                                       previous = vertex;
                                   }
                                                }

                                                void steepStraightLines() {
                                                    const auto viewport = makeViewport();
                                                    for (double slope : {500.0, -2500.0, 20000.0, -20000.0}) {
                                                        const auto f = [=](double x) { return slope * x + 0.13; };
                                                        const auto samples = sampleExplicitCurve(viewport, f);
                                                        require(!samples.truncated, "affine sampling is bounded");
                                                        require(continuousAcross(samples, viewport.worldToScreenX(0.0)),
                                                                "steep straight line stays connected at the origin");
                                                        std::size_t paths = 0;
                                                        for (const auto& vertex : samples.vertices) {
                                                            if (vertex.moveTo) ++paths;
                                                        }
                                                        require(paths == 1, "affine graph has one connected path");
                                                        checkChords(samples, viewport, f, 1e-6);
                                                    }
                                                }

                                                void continuousTransitions() {
                                                    // #17 produces two disconnected paths for these perfectly smooth,
                                                    // high-gradient functions around their inflection points.
                                                    for (double edge : {0.0, 0.137, -0.327}) {
                                                        const auto viewport = makeViewport();
                                                        const double sx = viewport.worldToScreenX(edge);
                                                        const auto tanhCurve = [=](double x) {
                                                            return std::tanh(1000.0 * (x - edge));
                                                        };
                                                        const auto atanCurve = [=](double x) {
                                                            return std::atan(1000.0 * (x - edge));
                                                        };
                                                        const std::function<double(double)> curves[] = {tanhCurve, atanCurve};
                                                        for (const auto& f : curves) {
                                                            const auto result = sampleExplicitCurve(viewport, f);
                                                            require(!result.truncated, "smooth transition stays within budget");
                                                            require(continuousAcross(result, sx),
                                                                    "smooth steep transition remains connected");
                                                            checkChords(result, viewport, f, 2.01);
                                                        }
                                                    }
                                                }

                                                void steepCurveWithTranslatedView() {
                                                    const auto viewport = makeViewport(2.0, 0.6, 250.0);
                                                    constexpr double center = 2.137;
                                                    const double sx = viewport.worldToScreenX(center);
                                                    const auto curve = [](double x) {
                                                        return std::tanh(800.0 * (x - center));
                                                    };
                                                    const auto result = sampleExplicitCurve(viewport, curve);
                                                    require(!result.truncated, "translated steep curve within budget");
                                                    require(continuousAcross(result, sx),
                                                            "translated and zoomed transition remains connected");
                                                    checkChords(result, viewport, curve, 2.01);
                                                }

                                                void transitionPhaseSweep() {
                                                    const auto viewport = makeViewport();
                                                    // Deliberately move the steep transition through 257 non-grid-aligned
                                                    // positions for three orders of steepness. A clean central example
                                                    // alone is not sufficient evidence for a sampling algorithm.
                                                    for (double gradient : {200.0, 1000.0, 10000.0}) {
                                                        for (int index = 0; index < 257; ++index) {
                                                            const double edge = -0.5 + index / 256.0;
                                                            const auto samples = sampleExplicitCurve(viewport, [=](double x) {
                                                                return std::tanh(gradient * (x - edge));
                                                            });
                                                            require(!samples.truncated, "phase sweep stays within budget");
                                                            require(continuousAcross(samples, viewport.worldToScreenX(edge)),
                                                                    "steep continuous phase sweep does not split");
                                                        }
                                                    }
                                                }

                                                void preserveAsymptotesAndSteps() {
                                                    const auto viewport = makeViewport();
                                                    for (double edge : {0.137, -0.327, 0.0}) {
                                                        const auto reciprocal = sampleExplicitCurve(viewport, [=](double x) {
                                                            return 1.0 / (x - edge);
                                                        });
                                                        const auto step = sampleExplicitCurve(viewport, [=](double x) {
                                                            return x < edge ? -1.0 : 1.0;
                                                        });
                                                        for (const auto* output : {&reciprocal, &step}) {
                                                            require(!output->truncated, "discontinuity stays within budget");
                                                            CurveVertex previous{};
                                                            bool started = false;
                                                            bool sawLeft = false;
                                                            bool sawRight = false;
                                                            for (const auto& current : output->vertices) {
                                                                if (current.moveTo) {
                                                                    previous = current;
                                                                    started = true;
                                                                    continue;
                                                                }
                                                                require(started, "discontinuous path begins with moveTo");
                                                                const double a = viewport.screenToWorldX(previous.screenX);
                                                                const double b = viewport.screenToWorldX(current.screenX);
                                                                require(!(a < edge && b >= edge),
                                                                        "no segment bridges a known pole or step");
                                                                if (b < edge) sawLeft = true;
                                                                if (a > edge) sawRight = true;
                                                                previous = current;
                                                            }
                                                            require(sawLeft && sawRight, "both branches remain drawable");
                                                        }
                                                    }
                                                }

                                                void recoveryLimits() {
                                                    const auto viewport = makeViewport();
                                                    const auto sharp = [](double x) { return std::tanh(1000.0*x); };

                                                    CurveSamplingOptions disabled;
                                                    disabled.maxTerminalRefinement = 0;
                                                    const auto before = sampleExplicitCurve(viewport, sharp, disabled);
                                                    require(!continuousAcross(before, viewport.worldToScreenX(0.0)),
                                                            "disabled recovery reproduces former small gap");

                                                    const auto after = sampleExplicitCurve(viewport, sharp);
                                                    require(continuousAcross(after, viewport.worldToScreenX(0.0)),
                                                            "enabled recovery closes the smooth gap");
                                                    require(after.evaluations > before.evaluations,
                                                            "terminal rescue performs targeted additional probes");

                                                    CurveSamplingOptions invalid;
                                                    invalid.maxTerminalRefinement = 17;
                                                    int invocations = 0;
                                                    const auto rejected = sampleExplicitCurve(viewport, [&](double) {
                                                        ++invocations;
                                                        return 1.0;
                                                    }, invalid);
                                                    require(rejected.evaluations == 0 && rejected.vertices.empty() &&
                                                    invocations == 0, "invalid recovery option rejected before sampling");

                                                    CurveSamplingOptions capped;
                                                    capped.maxEvaluations = 35;
                                                    const auto limited = sampleExplicitCurve(viewport, sharp, capped);
                                                    require(limited.truncated && limited.evaluations == capped.maxEvaluations,
                                                            "terminal refinement respects the existing evaluation ceiling");
                                                }

} // namespace

int main() {
    try {
        steepStraightLines();
        continuousTransitions();
        steepCurveWithTranslatedView();
        transitionPhaseSweep();
        preserveAsymptotesAndSteps();
        recoveryLimits();
        std::cout << "PASS: " << checks << " CurveSteep assertions\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
