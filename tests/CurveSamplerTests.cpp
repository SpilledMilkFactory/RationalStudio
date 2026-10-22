#include "CurveSampler.hpp"
#include "Expression.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace rationalstudio;

namespace {

    int assertions = 0;

    void expect(
        bool condition,
        const std::string& message
    ) {
        if (!condition) {
            throw std::runtime_error(message);
        }

        ++assertions;
    }

    void near(
        double actual,
        double expected,
        double tolerance,
        const std::string& message
    ) {
        expect(
            std::isfinite(actual) &&
            std::abs(actual - expected) <= tolerance,
               message
        );
    }

    void validateGeometry(
        const CurveSamples& result,
        const GraphViewport& view
    ) {
        bool hasStart = false;

        double previousX = 0.0;

        for (const auto& point : result.vertices) {
            expect(
                std::isfinite(point.screenX) &&
                std::isfinite(point.screenY),
                   "every generated point is finite"
            );

            expect(
                point.screenX >= 0.0 &&
                point.screenX <= view.width(),
                   "every generated point lies in the horizontal viewport"
            );

            if (point.moveTo) {
                hasStart = true;
            } else {
                expect(
                    hasStart,
                    "every lineTo has a preceding moveTo"
                );

                expect(
                    point.screenX >= previousX,
                    "sampled geometry has nondecreasing X"
                );
            }

            previousX = point.screenX;
        }
    }

} // namespace

int main() {
    try {
        GraphViewport view;

        expect(
            view.setDimensions(800, 600),
               "set dimensions"
        );

        expect(
            view.setView(0, 0, 100),
               "set view"
        );

        // -------------------------------------------------
        // Straight-line sampling
        // -------------------------------------------------

        const auto line = sampleExplicitCurve(
            view,
            [](double x) {
                return 2.0 * x + 1.0;
            }
        );

        expect(
            !line.vertices.empty(),
               "line has geometry"
        );

        expect(
            !line.truncated,
            "line stays within safety limit"
        );

        expect(
            line.evaluations < 400,
            "line needs fewer evaluations than M0 two-pixel step"
        );

        validateGeometry(line, view);

        near(
            line.vertices.front().screenX,
             0,
             1e-12,
             "left edge reached"
        );

        near(
            line.vertices.back().screenX,
             800,
             1e-12,
             "right edge reached"
        );

        for (const auto& vertex : line.vertices) {
            const double x = view.screenToWorldX(
                vertex.screenX
            );

            near(
                view.screenToWorldY(vertex.screenY),
                 2.0 * x + 1.0,
                 1e-10,
                 "line lies on equation"
            );
        }

        // -------------------------------------------------
        // Nonlinear curve sampling
        // -------------------------------------------------

        Expression sinExpression("sin(5*x)");

        expect(
            sinExpression.valid(),
               "sin expression valid"
        );

        const auto sine = sampleExplicitCurve(
            view,
            [&](double x) {
                return sinExpression.evaluate(x);
            }
        );

        expect(
            !sine.vertices.empty(),
               "nonlinear graph has geometry"
        );

        expect(
            !sine.truncated,
            "sine sampling finishes"
        );

        validateGeometry(sine, view);

        expect(
            sine.evaluations > line.evaluations,
            "curved graph receives more evaluations than straight line"
        );

        // -------------------------------------------------
        // Symmetric oscillation detection
        // -------------------------------------------------

        // A chord that covers a full oscillation looks flat
        // at endpoints AND midpoint.
        //
        // The quarter probes should catch this counterexample.

        const double periodPixels = 24.0;

        const auto oscillation = sampleExplicitCurve(
            view,
            [&](double worldX) {
                const double sx =
                view.worldToScreenX(worldX);

                return std::sin(
                    2.0 *
                    3.14159265358979323846 *
                    sx / periodPixels
                );
            }
        );

        expect(
            oscillation.evaluations > line.evaluations,
            "quarter probes detect a feature hidden at the midpoint"
        );

        validateGeometry(oscillation, view);

        // -------------------------------------------------
        // Approximation accuracy
        // -------------------------------------------------

        // Check the screen-space midpoint deviation
        // for every generated line on a smooth function.
        //
        // Tight error tolerance is configuration-driven.

        bool havePrevious = false;

        CurveVertex previous;

        for (const auto& vertex : sine.vertices) {
            if (vertex.moveTo) {
                havePrevious = true;

                previous = vertex;

                continue;
            }

            expect(
                havePrevious,
                "segment begins after moveTo"
            );

            const double sx =
            (
                previous.screenX +
                vertex.screenX
            ) * 0.5;

            const double sy =
            view.worldToScreenY(
                sinExpression.evaluate(
                    view.screenToWorldX(sx)
                )
            );

            const double approximate =
            (
                previous.screenY +
                vertex.screenY
            ) * 0.5;

            near(
                approximate,
                 sy,
                 2.0,
                 "smooth curve midpoint approximation"
            );

            previous = vertex;
        }

        // -------------------------------------------------
        // Undefined mathematical domains
        // -------------------------------------------------

        const auto sqrt = sampleExplicitCurve(
            view,
            [](double x) {
                return std::sqrt(x);
            }
        );

        validateGeometry(sqrt, view);

        expect(
            !sqrt.vertices.empty(),
               "sqrt has drawable geometry"
        );

        for (const auto& vertex : sqrt.vertices) {
            expect(
                view.screenToWorldX(vertex.screenX) >= -1e-12,
                   "sqrt never generates negative-domain geometry"
            );
        }

        const auto undefined = sampleExplicitCurve(
            view,
            [](double) {
                return std::numeric_limits<double>::
                quiet_NaN();
            }
        );

        expect(
            undefined.vertices.empty(),
               "entirely undefined domain yields no geometry"
        );

        expect(
            !undefined.truncated,
            "undefined domain respects termination guard"
        );

        // -------------------------------------------------
        // Sampling safety limit
        // -------------------------------------------------

        // Hard safety cap is a safeguard, not the full graph
        // workload policy scheduled for issue #21.

        CurveSamplingOptions limited;

        limited.maxEvaluations = 20;

        const auto bounded = sampleExplicitCurve(
            view,
            [](double x) {
                return std::sin(5 * x);
            },
            limited
        );

        expect(
            bounded.truncated,
            "sampling limit is observable"
        );

        expect(
            bounded.evaluations == 20,
            "sampling does not exceed its limit"
        );

        // -------------------------------------------------
        // Invalid configuration
        // -------------------------------------------------

        // No callbacks should run with invalid configuration.

        int calls = 0;

        CurveSamplingOptions bad;

        bad.coarseStepPixels = -1;

        const auto invalid = sampleExplicitCurve(
            view,
            [&](double) {
                ++calls;
                return 1.0;
            },
            bad
        );

        expect(
            invalid.vertices.empty() && calls == 0,
               "invalid options fail without evaluating"
        );

        // -------------------------------------------------
        // Results
        // -------------------------------------------------

        std::cout
        << "PASS: "
        << assertions
        << " CurveSampler assertions\n"
        << "linear evaluations: "
        << line.evaluations << "\n"
        << "sine evaluations: "
        << sine.evaluations << "\n"
        << "oscillation evaluations: "
        << oscillation.evaluations << "\n";

        return 0;

    } catch (const std::exception& error) {
        std::cerr
        << "FAIL: "
        << error.what()
        << '\n';

        return 1;
    }
}
