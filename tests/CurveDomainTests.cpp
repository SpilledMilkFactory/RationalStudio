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

    std::size_t checks = 0;

    void require(
        bool condition,
        const std::string& message
    ) {
        if (!condition) {
            throw std::runtime_error(message);
        }

        ++checks;
    }

    template <class F>
    void forEachLine(
        const CurveSamples& samples,
        F callback
    ) {
        bool haveStart = false;

        CurveVertex previous{};

        for (const CurveVertex& current : samples.vertices) {
            require(
                std::isfinite(current.screenX) &&
                std::isfinite(current.screenY),
                    "finite drawing vertices"
            );

            require(
                std::abs(current.screenY) <= 1e7,
                    "bounded drawing coordinate"
            );

            if (current.moveTo) {
                previous = current;
                haveStart = true;
                continue;
            }

            require(
                haveStart,
                "lineTo requires a preceding moveTo"
            );

            require(
                current.screenX > previous.screenX,
                "positive horizontal progress on each segment"
            );

            callback(previous, current);

            previous = current;
        }
    }

    std::size_t pathCount(
        const CurveSamples& samples
    ) {
        return static_cast<std::size_t>(
            std::count_if(
                samples.vertices.begin(),
                          samples.vertices.end(),
                          [](const CurveVertex& v) {
                              return v.moveTo;
                          }
            )
        );
    }

    void requireBoundedDomain(
        const CurveSamples& samples,
        const GraphViewport& view,
        double minX,
        double maxX,
        const std::string& label
    ) {
        forEachLine(
            samples,
            [&](const CurveVertex& a,
                const CurveVertex& b) {
                const double x0 =
                view.screenToWorldX(a.screenX);

                const double x1 =
                view.screenToWorldX(b.screenX);

                require(
                    x0 >= minX - 1e-12 &&
                    x0 <= maxX + 1e-12 &&
                    x1 >= minX - 1e-12 &&
                    x1 <= maxX + 1e-12,
                    label + ": line must remain in defined domain"
                );
                }
        );
    }

    void requireGap(
        const CurveSamples& samples,
        const GraphViewport& view,
        double gapLeft,
        double gapRight,
        const std::string& label
    ) {
        forEachLine(
            samples,
            [&](const CurveVertex& a,
                const CurveVertex& b) {
                const double x0 =
                view.screenToWorldX(a.screenX);

                const double x1 =
                view.screenToWorldX(b.screenX);

                // No rendered segment may intersect
                // a positive-length subset of the
                // known open interval where the
                // function is undefined.

                const double intersectionLeft =
                std::max(x0, gapLeft);

                const double intersectionRight =
                std::min(x1, gapRight);

                require(
                    intersectionRight <=
                    intersectionLeft + 1e-12,
                    label + ": line crosses an undefined interval"
                );
                }
        );
    }

    CurveSamples fromExpression(
        const GraphViewport& viewport,
        const std::string& formula
    ) {
        Expression expression(formula);

        require(
            expression.valid(),
                "test formula parses: " + formula
        );

        return sampleExplicitCurve(
            viewport,
            [&](double x) {
                return expression.evaluate(x);
            }
        );
    }

} // namespace

int main() {
    try {
        GraphViewport view;

        require(
            view.setDimensions(800, 600),
                "viewport dimensions"
        );

        require(
            view.setView(0, 0, 100),
                "viewport scale"
        );

        // ==================================================
        // Square root: closed domain boundary
        // ==================================================
        //
        // x = 0 belongs to sqrt(x)'s domain.
        // Negative x must not generate drawing geometry.

        const auto root = fromExpression(
            view,
            "sqrt(x)"
        );

        require(
            !root.truncated,
            "sqrt sampling completes"
        );

        require(
            !root.vertices.empty(),
                "sqrt graph exists"
        );

        requireBoundedDomain(
            root,
            view,
            0.0,
            4.0,
            "sqrt(x)"
        );

        require(
            root.vertices.front().screenX >=
            400.0 - 1e-8,
            "sqrt does not extend to negative x"
        );

        // ==================================================
        // Natural logarithm: open domain boundary
        // ==================================================
        //
        // ln(x) is undefined for x <= 0.

        const auto log = fromExpression(
            view,
            "ln(x)"
        );

        require(
            !log.truncated,
            "log sampling completes"
        );

        require(
            !log.vertices.empty(),
                "log graph exists"
        );

        requireBoundedDomain(
            log,
            view,
            0.0,
            4.0,
            "ln(x)"
        );

        for (const auto& vertex : log.vertices) {
            require(
                view.screenToWorldX(
                    vertex.screenX
                ) > 0.0,
                "ln(x) never emits x<=0"
            );
        }

        // ==================================================
        // Inverse square root
        // ==================================================
        //
        // This function is defined only for x > 0.
        // It also grows without bound near x = 0.

        const auto inverseRoot = fromExpression(
            view,
            "1/sqrt(x)"
        );

        require(
            !inverseRoot.vertices.empty(),
                "inverse root graph exists"
        );

        requireBoundedDomain(
            inverseRoot,
            view,
            0.0,
            4.0,
            "1/sqrt(x)"
        );

        for (const auto& vertex : inverseRoot.vertices) {
            require(
                view.screenToWorldX(
                    vertex.screenX
                ) > 0.0,
                "inverse sqrt never emits x<=0"
            );
        }

        // ==================================================
        // Inverse cosine: two finite domain boundaries
        // ==================================================
        //
        // acos(x) is defined over [-1, 1].

        const auto cosineInverse = fromExpression(
            view,
            "acos(x)"
        );

        require(
            !cosineInverse.vertices.empty(),
                "acos graph exists"
        );

        requireBoundedDomain(
            cosineInverse,
            view,
            -1.0,
            1.0,
            "acos(x)"
        );

        // ==================================================
        // Bounded domain: a finite mathematical island
        // ==================================================
        //
        // Valid only between two non-grid-aligned bounds.

        constexpr double domainLeft = -0.537;
        constexpr double domainRight = 0.423;

        const auto island = sampleExplicitCurve(
            view,
            [](double x) {
                return std::sqrt(
                    (x - domainLeft) *
                    (domainRight - x)
                );
            }
        );

        require(
            !island.truncated,
            "bounded island sampling completes"
        );

        require(
            !island.vertices.empty(),
                "bounded island is discovered"
        );

        requireBoundedDomain(
            island,
            view,
            domainLeft,
            domainRight,
            "finite domain island"
        );

        require(
            pathCount(island) >= 1,
                "bounded island has a path"
        );

        // ==================================================
        // Narrow undefined hole between valid branches
        // ==================================================
        //
        // The boundaries are deliberately not aligned
        // with integer screen coordinates.

        constexpr double gapLeft = 0.131;
        constexpr double gapRight = 0.147;

        const auto hole = sampleExplicitCurve(
            view,
            [](double x) {
                return std::sqrt(
                    (x - gapLeft) *
                    (x - gapRight)
                );
            }
        );

        require(
            !hole.truncated,
            "hole sampling completes"
        );

        requireGap(
            hole,
            view,
            gapLeft,
            gapRight,
            "sqrt hole"
        );

        require(
            pathCount(hole) >= 2,
                "valid branches remain disconnected"
        );

        // ==================================================
        // Regression: sub-pixel undefined interval
        // ==================================================
        //
        // The M1-02 sampler checked only midpoints
        // after an interval became smaller than
        // six pixels.
        //
        // This allowed a particular narrow domain
        // pocket to be connected across.
        //
        // The updated implementation must preserve
        // a separation between its valid branches.

        constexpr double pocketStartPixel = 426.2;
        constexpr double pocketEndPixel = 427.0;

        const double pocketLeft =
        view.screenToWorldX(pocketStartPixel);

        const double pocketRight =
        view.screenToWorldX(pocketEndPixel);

        const auto pocket = sampleExplicitCurve(
            view,
            [&](double x) {
                if (
                    x > pocketLeft &&
                    x < pocketRight
                ) {
                    return std::numeric_limits<double>::
                    quiet_NaN();
                }

                return std::sin(5.0 * x);
            }
        );

        require(
            !pocket.truncated,
            "small domain pocket completes"
        );

        requireGap(
            pocket,
            view,
            pocketLeft,
            pocketRight,
            "sub-pixel undefined pocket"
        );

        require(
            pathCount(pocket) >= 2,
                "sub-pixel undefined pocket separates branches"
        );

        // ==================================================
        // Exception-producing mathematical callbacks
        // ==================================================
        //
        // Exceptions during evaluation must be
        // treated as undefined samples rather than
        // propagating into the graph renderer.

        const auto throwing = sampleExplicitCurve(
            view,
            [](double x) -> double {
                if (
                    x > gapLeft &&
                    x < gapRight
                ) {
                    throw std::domain_error(
                        "undefined domain"
                    );
                }

                return 0.0;
            }
        );

        require(
            !throwing.truncated,
            "throwing callback completes"
        );

        requireGap(
            throwing,
            view,
            gapLeft,
            gapRight,
            "throwing hole"
        );

        require(
            pathCount(throwing) >= 2,
                "throwing domain splits paths"
        );

        // ==================================================
        // Non-finite and extreme drawing values
        // ==================================================
        //
        // None of these values should produce
        // drawable geometry.

        for (double value : {
            std::numeric_limits<double>::quiet_NaN(),
             std::numeric_limits<double>::infinity(),
             -std::numeric_limits<double>::infinity(),
             1e308
        }) {
            const auto unrenderable = sampleExplicitCurve(
                view,
                [value](double) {
                    return value;
                }
            );

            require(
                unrenderable.vertices.empty(),
                    "unrenderable values produce no geometry"
            );

            require(
                !unrenderable.truncated,
                "unrenderable values terminate normally"
            );
        }

        // ==================================================
        // Evaluation safety limit
        // ==================================================
        //
        // Pathological domains must still respect
        // the sampler's configured work limit.

        CurveSamplingOptions limited;

        limited.maxEvaluations = 25;

        const auto bounded = sampleExplicitCurve(
            view,
            [](double) {
                return std::numeric_limits<double>::
                quiet_NaN();
            },
            limited
        );

        require(
            bounded.truncated,
            "domain scan honors evaluation limit"
        );

        require(
            bounded.evaluations == 25,
            "domain scan never exceeds evaluation limit"
        );

        // ==================================================
        // Results
        // ==================================================

        std::cout
        << "PASS: "
        << checks
        << " CurveDomain assertions\n";

        return 0;

    } catch (const std::exception& error) {
        std::cerr
        << "FAIL: "
        << error.what()
        << '\n';

        return 1;
    }
}
