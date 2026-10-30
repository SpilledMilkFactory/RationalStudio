#include "NumericalRootFinding.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <utility>

namespace rationalstudio {
namespace {

bool validRootOptions(const RootOptions& o) noexcept {
    return std::isfinite(o.xAbsoluteTolerance) && o.xAbsoluteTolerance > 0 &&
           std::isfinite(o.xRelativeTolerance) && o.xRelativeTolerance >= 0 &&
           std::isfinite(o.residualTolerance) && o.residualTolerance > 0 &&
           o.maxIterations >= 1 && o.maxIterations <= 100000 &&
           o.maxEvaluations >= 2 && o.maxEvaluations <= 10000000;
}

bool validBounds(double a, double b) noexcept {
    return std::isfinite(a) && std::isfinite(b) && a < b &&
           std::isfinite(b - a);
}

bool opposed(double a, double b) noexcept {
    return (a < 0 && b > 0) || (a > 0 && b < 0);
}

class Budget {
public:
    Budget(const RootFunction& fn, std::size_t limit)
        : fn_(fn), limit_(limit) {}

    // Count invalid/throwing callbacks as real evaluations.
    bool evaluate(double x, double& y) {
        if (used_ >= limit_) return false;
        ++used_;
        y = std::numeric_limits<double>::quiet_NaN();
        try { y = fn_(x); } catch (...) { /* domain/callback failure */ }
        return true;
    }

    [[nodiscard]] std::size_t used() const noexcept { return used_; }
    [[nodiscard]] std::size_t remaining() const noexcept { return limit_ - used_; }
private:
    const RootFunction& fn_;
    std::size_t limit_;
    std::size_t used_ = 0;
};

RootResult refine(Budget& budget, double a, double b, double fa, double fb,
                  const RootOptions& options) {
    RootResult r;
    r.bracketLower = a;
    r.bracketUpper = b;
    if (!std::isfinite(fa) || !std::isfinite(fb)) {
        r.status = RootStatus::UndefinedEndpoint;
        return r;
    }
    auto accept = [&](double x, double fx) {
        r.status = RootStatus::Converged;
        r.x = x;
        r.residual = std::abs(fx);
    };
    if (std::abs(fa) <= options.residualTolerance) {
        accept(a, fa);
        return r;
    }
    if (std::abs(fb) <= options.residualTolerance) {
        accept(b, fb);
        return r;
    }
    if (!opposed(fa, fb)) {
        r.status = RootStatus::NoSignChange;
        return r;
    }

    for (std::size_t iteration = 0; iteration < options.maxIterations; ++iteration) {
        const double width = b - a;
        // Relative coordinate scale matters near large finite offsets.
        const double tolerance = std::max(options.xAbsoluteTolerance,
            options.xRelativeTolerance * std::max(std::abs(a), std::abs(b)));
        const double mid = std::midpoint(a, b);
        r.bracketLower = a;
        r.bracketUpper = b;
        if (!(mid > a && mid < b) || width <= tolerance) {
            r.status = RootStatus::Unresolved; // width alone is NOT a root
            return r;
        }

        // Secant position with scaled values to avoid overflow in fb-fa.
        const double scale = std::max(std::abs(fa), std::abs(fb));
        const double sa = fa / scale, sb = fb / scale;
        const double fraction = sa / (sa - sb);
        double trial = std::fma(fraction, width, a);
        // Guaranteed bracket shrink even if interpolation stagnates.
        if (!std::isfinite(trial) ||
            !(trial > std::fma(0.10, width, a) &&
              trial < std::fma(0.90, width, a)))
            trial = mid;
        if (!(trial > a && trial < b)) {
            r.status = RootStatus::Unresolved;
            return r;
        }
        double ft = 0.0;
        // In domain-search mode, also respect the per-bracket refinement cap.
        // The two already-sampled endpoints count toward this allowance.
        if (r.iterations + 2 >= options.maxEvaluations ||
            !budget.evaluate(trial, ft)) {
            r.status = RootStatus::EvaluationLimit;
            return r;
        }
        ++r.iterations;
        if (!std::isfinite(ft)) {
            r.status = RootStatus::UndefinedDuringSearch;
            return r;
        }
        if (std::abs(ft) <= options.residualTolerance) {
            if (opposed(fa, ft)) b = trial;
            else if (opposed(ft, fb)) a = trial;
            r.bracketLower = a;
            r.bracketUpper = b;
            accept(trial, ft);
            return r;
        }
        if (opposed(fa, ft)) {
            b = trial;
            fb = ft;
        } else if (opposed(ft, fb)) {
            a = trial;
            fa = ft;
        } else {
            r.status = RootStatus::Unresolved;
            return r;
        }
    }
    r.bracketLower = a;
    r.bracketUpper = b;
    r.status = RootStatus::IterationLimit;
    return r;
}

bool validSearchOptions(const RootSearchOptions& o) noexcept {
    return validRootOptions(o.root) && o.subdivisions >= 2 &&
        o.subdivisions <= 65536 && o.maxResults >= 1 &&
        o.maxResults <= 10000 && o.maxEvaluations >= 3 &&
        o.maxEvaluations <= 10000000;
}

} // namespace

RootResult solveBracketedRoot(const RootFunction& fn, double lower,
                              double upper, const RootOptions& options) {
    RootResult r;
    if (!fn || !validBounds(lower, upper) || !validRootOptions(options)) return r;
    Budget budget(fn, options.maxEvaluations);
    double fa = 0.0, fb = 0.0;
    if (!budget.evaluate(lower, fa) || !budget.evaluate(upper, fb)) {
        r.status = RootStatus::EvaluationLimit;
        r.evaluations = budget.used();
        return r;
    }
    r = refine(budget, lower, upper, fa, fb, options);
    r.evaluations = budget.used();
    return r;
}

RootSearchResult findRootsInDomain(const RootFunction& fn, double lower,
                                  double upper, const RootSearchOptions& options) {
    RootSearchResult out;
    if (!fn || !validBounds(lower, upper) || !validSearchOptions(options)) return out;
    Budget budget(fn, options.maxEvaluations);
    const double width = upper - lower;
    out.status = RootSearchStatus::Complete;
    bool incomplete = false;

    auto add = [&](RootResult root) {
        if (!root.converged()) return;
        const double tolerance = std::max(options.root.xAbsoluteTolerance * 4.0,
            options.root.xRelativeTolerance * std::abs(root.x) * 4.0);
        for (const auto& old : out.roots)
            if (std::abs(old.x - root.x) <= tolerance) return;
        if (out.roots.size() >= options.maxResults) {
            out.status = RootSearchStatus::ResultLimit;
            return;
        }
        out.roots.push_back(root);
    };

    double previousX = lower, previousF = 0;
    bool previousValid = false;
    for (std::size_t i = 0; i <= options.subdivisions; ++i) {
        const double x = i == options.subdivisions ? upper :
            std::fma(static_cast<double>(i) / options.subdivisions, width, lower);
        // Large coordinates can collapse adjacent grid points. Do not invent
        // extra samples or intervals between identical representable numbers.
        if (i > 0 && x <= previousX) {
            incomplete = true;
            continue;
        }
        double f = 0.0;
        if (!budget.evaluate(x, f)) {
            out.status = RootSearchStatus::EvaluationLimit;
            break;
        }
        const bool currentValid = std::isfinite(f);
        if (!currentValid) {
            ++out.invalidSamples;
            incomplete = true;
        } else {
            // Sampled zero: a numerical residual test, not root certification.
            if (std::abs(f) <= options.root.residualTolerance) {
                RootResult sample;
                sample.status = RootStatus::Converged;
                sample.x = x;
                sample.residual = std::abs(f);
                sample.bracketLower = x;
                sample.bracketUpper = x;
                add(sample);
            }
            if (previousValid && opposed(previousF, f)) {
                // Each refinement consumes the SAME global callback budget.
                RootResult found = refine(budget, previousX, x, previousF, f,
                                          options.root);
                found.evaluations = budget.used();
                if (found.converged()) add(found);
                else {
                    ++out.rejectedBrackets;
                    incomplete = true;
                    if (found.status == RootStatus::EvaluationLimit)
                        out.status = RootSearchStatus::EvaluationLimit;
                }
            }
        }
        previousX = x;
        previousF = f;
        previousValid = currentValid;
        if (out.status == RootSearchStatus::EvaluationLimit ||
            out.status == RootSearchStatus::ResultLimit) break;
    }
    out.evaluations = budget.used();
    std::sort(out.roots.begin(), out.roots.end(),
              [](const RootResult& a, const RootResult& b) { return a.x < b.x; });
    if (out.status == RootSearchStatus::Complete && incomplete)
        out.status = RootSearchStatus::IncompleteDomain;
    return out;
}

RootResult solveBracketedRoot(const Expression& expression, double lower,
                              double upper, AngleMode angle,
                              const VariableBindings& bindings, double ans,
                              const RootOptions& options) {
    if (!expression.valid() || !std::isfinite(ans)) return {};
    const VariableBindings snapshot = bindings;
    return solveBracketedRoot([&](double x) {
        return expression.evaluate(x, ans, angle, snapshot);
    }, lower, upper, options);
}

RootResult solveEquation(const Expression& left, const Expression& right,
                         double lower, double upper, AngleMode angle,
                         const VariableBindings& bindings, double ans,
                         const RootOptions& options) {
    if (!left.valid() || !right.valid() || !std::isfinite(ans)) return {};
    const VariableBindings snapshot = bindings;
    return solveBracketedRoot([&](double x) {
        return left.evaluate(x, ans, angle, snapshot) -
               right.evaluate(x, ans, angle, snapshot);
    }, lower, upper, options);
}

RootSearchResult findRootsInDomain(const Expression& expression, double lower,
                                  double upper, AngleMode angle,
                                  const VariableBindings& bindings, double ans,
                                  const RootSearchOptions& options) {
    if (!expression.valid() || !std::isfinite(ans)) return {};
    const VariableBindings snapshot = bindings;
    return findRootsInDomain([&](double x) {
        return expression.evaluate(x, ans, angle, snapshot);
    }, lower, upper, options);
}

const char* rootStatusName(RootStatus status) noexcept {
    switch (status) {
    case RootStatus::Converged: return "converged";
    case RootStatus::InvalidInput: return "invalid input";
    case RootStatus::UndefinedEndpoint: return "undefined endpoint";
    case RootStatus::NoSignChange: return "no sign change";
    case RootStatus::UndefinedDuringSearch: return "undefined during search";
    case RootStatus::EvaluationLimit: return "evaluation limit";
    case RootStatus::IterationLimit: return "iteration limit";
    case RootStatus::Unresolved: return "unresolved";
    }
    return "unknown";
}
const char* rootSearchStatusName(RootSearchStatus status) noexcept {
    switch (status) {
    case RootSearchStatus::Complete: return "complete finite scan";
    case RootSearchStatus::InvalidInput: return "invalid input";
    case RootSearchStatus::IncompleteDomain: return "incomplete or undefined domain";
    case RootSearchStatus::EvaluationLimit: return "evaluation limit";
    case RootSearchStatus::ResultLimit: return "result limit";
    }
    return "unknown";
}
const char* rootMethodName(RootMethod method) noexcept {
    switch (method) {
    case RootMethod::SafeguardedSecantBisection: return "safeguarded secant/bisection";
    }
    return "unknown";
}

} // namespace rationalstudio
