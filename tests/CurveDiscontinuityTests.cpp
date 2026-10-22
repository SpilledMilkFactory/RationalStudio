#include "CurveSampler.hpp"
#include "Expression.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace rationalstudio;

namespace {

    std::size_t checks = 0;

    constexpr double pi =
    3.14159265358979323846264338327950288;

    void expect(
        bool value,
        const std::string& what
    ) {
        if (!value) {
            throw std::runtime_error(what);
        }

        ++checks;
    }

    GraphViewport makeView(
        double centerX = 0.0,
        double scale = 100.0
    ) {
        GraphViewport v;

        expect(
            v.setDimensions(800, 600),
               "valid dimensions"
        );

        expect(
            v.setView(centerX, 0.0, scale),
               "valid view"
        );

        return v;
    }

    // Verify actual rendered line segments,
    // not merely mathematical evaluation results.
    //
    // A moveTo begins a new drawing path and
    // must not be interpreted as a connection.

    template <class Fn>
    void forEachSegment(
        const CurveSamples& samples,
        Fn&& callback
    ) {
        CurveVertex previous{};

        bool havePrevious = false;

        for (const CurveVertex& vertex : samples.vertices) {
            expect(
                std::isfinite(vertex.screenX) &&
                std::isfinite(vertex.screenY),
                   "finite drawing geometry"
            );

            if (vertex.moveTo) {
                previous = vertex;
                havePrevious = true;
                continue;
            }

            expect(
                havePrevious,
                "line must start with moveTo"
            );

            expect(
                vertex.screenX > previous.screenX,
                "positive X progress"
            );

            callback(previous, vertex);

            previous = vertex;
        }
    }

    void expectNoCrossing(
        const CurveSamples& samples,
        const GraphViewport& viewport,
        double discontinuity,
        const std::string& name
    ) {
        expect(
            !samples.truncated,
            name + ": sampling did not truncate"
        );

        expect(
            !samples.vertices.empty(),
               name + ": geometry generated"
        );

        bool sawLeft = false;
        bool sawRight = false;

        forEachSegment(
            samples,
            [&](const CurveVertex& a,
                const CurveVertex& b) {

                const double left =
                viewport.screenToWorldX(a.screenX);

                const double right =
                viewport.screenToWorldX(b.screenX);

                // Exclude a segment that crosses
                // a known discontinuity.
                //
                // Also exclude a segment that reaches
                // the discontinuity from the wrong side
                // when the function jumps at that x.

                expect(
                    !(
                        left < discontinuity &&
                        right >= discontinuity
                    ),
                    name + ": segment bridges discontinuity"
                );

                if (right < discontinuity) {
                    sawLeft = true;
                }

                if (left > discontinuity) {
                    sawRight = true;
                }
                }
        );

        expect(
            sawLeft && sawRight,
            name + ": both branches remain visible"
        );
    }

    // ---------------------------------------------------------
    // Reciprocal Function Tests
    // ---------------------------------------------------------

    void reciprocalCases() {
        const auto view = makeView();

        for (const double pole : {
            0.133,
            -0.327,
            1.333,
            0.0
        }) {
            const auto samples = sampleExplicitCurve(
                view,
                [pole](double x) {
                    return 1.0 / (x - pole);
                }
            );

            expectNoCrossing(
                samples,
                view,
                pole,
                "reciprocal pole"
            );
        }
    }

    // ---------------------------------------------------------
    // Tangent Asymptote Tests
    // ---------------------------------------------------------

    void tangentCases() {
        const auto view = makeView();

        Expression expr("tan(x)");

        expect(
            expr.valid(),
               "tan expression parses"
        );

        const auto samples = sampleExplicitCurve(
            view,
            [&](double x) {
                return expr.evaluate(x);
            }
        );

        for (double pole : {
            -pi / 2.0,
            pi / 2.0
        }) {
            expectNoCrossing(
                samples,
                view,
                pole,
                "tangent pole"
            );
        }
    }

    // ---------------------------------------------------------
    // Finite Jump Discontinuity Tests
    // ---------------------------------------------------------

    void finiteJumpCases() {
        const auto view = makeView();

        constexpr double edge = 0.137;

        const auto step = sampleExplicitCurve(
            view,
            [](double x) {
                return x < edge ? -1.0 : 1.0;
            }
        );

        expectNoCrossing(
            step,
            view,
            edge,
            "finite step jump"
        );

        Expression floor("floor(x)");

        expect(
            floor.valid(),
               "floor expression parses"
        );

        const auto stairs = sampleExplicitCurve(
            view,
            [&](double x) {
                return floor.evaluate(x);
            }
        );

        for (double boundary : {
            -2.0,
            -1.0,
            0.0,
            1.0,
            2.0
        }) {
            expectNoCrossing(
                stairs,
                view,
                boundary,
                "integer step"
            );
        }
    }

    // ---------------------------------------------------------
    // Sampling Phase Regression Tests
    // ---------------------------------------------------------

    void samplePhaseSweep() {
        const auto view = makeView();

        // Sweep pole and step locations through
        // non-grid-aligned sampling positions.
        //
        // These are deterministic regression cases,
        // not a proof for every possible coordinate.

        for (int i = 0; i < 257; ++i) {
            const double edge =
            -2.75 + i * 5.5 / 256.0;

            const auto reciprocal = sampleExplicitCurve(
                view,
                [edge](double x) {
                    return 1.0 / (x - edge);
                }
            );

            const auto step = sampleExplicitCurve(
                view,
                [edge](double x) {
                    return x < edge ? -1.0 : 1.0;
                }
            );

            auto bridge = [&](const CurveSamples& samples) {
                CurveVertex previous{};

                bool havePrevious = false;

                for (const auto& vertex : samples.vertices) {
                    if (vertex.moveTo) {
                        previous = vertex;
                        havePrevious = true;
                        continue;
                    }

                    if (
                        havePrevious &&
                        view.screenToWorldX(
                            previous.screenX
                        ) < edge &&
                        view.screenToWorldX(
                            vertex.screenX
                        ) >= edge
                    ) {
                        return true;
                    }

                    previous = vertex;
                }

                return false;
            };

            expect(
                !reciprocal.truncated &&
                !bridge(reciprocal),
                   "sample-phase reciprocal pole remains disconnected"
            );

            expect(
                !step.truncated &&
                !bridge(step),
                   "sample-phase finite step remains disconnected"
            );
        }
    }

    // ---------------------------------------------------------
    // Continuous Curve Control Tests
    // ---------------------------------------------------------

    void continuousControlCases() {
        const auto view = makeView();

        // A large vertical pixel change is NOT
        // proof of discontinuity.
        //
        // The previous height()*0.7 safeguard
        // incorrectly omitted this line.

        const auto steep = sampleExplicitCurve(
            view,
            [](double x) {
                return 500.0 * x + 17.0;
            }
        );

        expect(
            !steep.truncated &&
            !steep.vertices.empty(),
               "steep line sampled"
        );

        bool crossedCenter = false;

        forEachSegment(
            steep,
            [&](const CurveVertex& a,
                const CurveVertex& b) {

                if (
                    a.screenY >= 300.0 &&
                    b.screenY <= 300.0
                ) {
                    crossedCenter = true;
                }

                const double midpointX =
                (
                    a.screenX +
                    b.screenX
                ) * 0.5;

                const double expected =
                view.worldToScreenY(
                    500.0 *
                    view.screenToWorldX(midpointX) +
                    17.0
                );

                expect(
                    std::abs(
                        (a.screenY + b.screenY) * 0.5 -
                        expected
                    ) < 1e-7,
                    "steep linear interpolation remains accurate"
                );
                }
        );

        expect(
            crossedCenter,
            "steep continuous line is not erased"
        );

        std::size_t subpaths = 0;

        for (const auto& vertex : steep.vertices) {
            if (vertex.moveTo) {
                ++subpaths;
            }
        }

        expect(
            subpaths == 1,
            "steep straight line stays connected"
        );

        // Ensure ordinary smooth nonlinear
        // functions remain drawable.

        const auto parabola = sampleExplicitCurve(
            view,
            [](double x) {
                return x * x;
            }
        );

        expect(
            !parabola.truncated &&
            !parabola.vertices.empty(),
               "parabola sampled"
        );

        forEachSegment(
            parabola,
            [&](const CurveVertex& a,
                const CurveVertex& b) {

                const double sx =
                (
                    a.screenX +
                    b.screenX
                ) * 0.5;

                const double expected =
                view.worldToScreenY(
                    std::pow(
                        view.screenToWorldX(sx),
                             2.0
                    )
                );

                expect(
                    std::abs(
                        (a.screenY + b.screenY) * 0.5 -
                        expected
                    ) < 3.0,
                    "parabola local screen-space error reasonable"
                );
                }
        );
    }

    // ---------------------------------------------------------
    // Panned and Zoomed Viewport Tests
    // ---------------------------------------------------------

    void translatedViewportCase() {
        const auto view = makeView(
            2.0,
            250.0
        );

        constexpr double pole = 2.137;

        const auto samples = sampleExplicitCurve(
            view,
            [](double x) {
                return 1.0 / (x - pole);
            }
        );

        expectNoCrossing(
            samples,
            view,
            pole,
            "panned/zoomed pole"
        );
    }

} // namespace

int main() {
    try {
        reciprocalCases();

        tangentCases();

        finiteJumpCases();

        continuousControlCases();

        samplePhaseSweep();

        translatedViewportCase();

        std::cout
        << "PASS: "
        << checks
        << " CurveDiscontinuity assertions\n";

        return 0;

    } catch (const std::exception& error) {
        std::cerr
        << "FAIL: "
        << error.what()
        << '\n';

        return 1;
    }
}
