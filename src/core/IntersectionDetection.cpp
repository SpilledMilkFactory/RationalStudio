#include "IntersectionDetection.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <utility>

namespace rationalstudio {
namespace {
struct Probe {
    double x = 0;
    double f = std::numeric_limits<double>::quiet_NaN();
    double g = std::numeric_limits<double>::quiet_NaN();
    double difference = std::numeric_limits<double>::quiet_NaN();
    [[nodiscard]] bool valid() const { return std::isfinite(difference); }
};

class Detection {
public:
    Detection(const CartesianIntersectionFunction& first,
              const CartesianIntersectionFunction& second,
              const IntersectionOptions& options)
        : first_(first), second_(second), options_(options) {}

    IntersectionResult execute(double minimum, double maximum) {
        if (!first_ || !second_ || !std::isfinite(minimum) ||
            !std::isfinite(maximum) || !(minimum < maximum) ||
            options_.subdivisions < 4 || options_.subdivisions > 65536 ||
            options_.maxEvaluations < 2 || options_.maxEvaluations > 10000000 ||
            options_.maxResults == 0 || options_.maxResults > 10000 ||
            !std::isfinite(options_.xTolerance) || options_.xTolerance <= 0 ||
            !std::isfinite(options_.residualTolerance) || options_.residualTolerance <= 0 ||
            !std::isfinite(maximum - minimum))
            return result_;
        result_.status = IntersectionStatus::Complete;
        const double step = (maximum - minimum) / static_cast<double>(options_.subdivisions);
        if (!(step > 0) || !std::isfinite(step)) {
            result_.status = IntersectionStatus::InvalidInput;
            return result_;
        }
        std::vector<Probe> grid;
        grid.reserve(options_.subdivisions + 1);
        std::size_t valid = 0;
        bool allZero = true;
        for (std::size_t i = 0; i <= options_.subdivisions; ++i) {
            Probe p;
            const double x = i == options_.subdivisions ? maximum :
                std::fma(static_cast<double>(i), step, minimum);
            if (!probe(x, p)) {
                result_.status = IntersectionStatus::EvaluationLimit;
                return result_;
            }
            if (p.valid()) {
                ++valid;
                allZero &= std::abs(p.difference) <= options_.residualTolerance;
            } else {
                allZero = false;
            }
            grid.push_back(p);
        }
        if (valid == grid.size() && allZero) {
            result_.status = IntersectionStatus::Coincident;
            return result_;
        }
        for (std::size_t i = 0; i < grid.size(); ++i) {
            if (!grid[i].valid()) continue;
            if (grid[i].difference == 0 ||
                ((i == 0 || i + 1 == grid.size()) &&
                 std::abs(grid[i].difference) <= options_.residualTolerance))
                add(grid[i], 0);
            if (stopped()) break;
            if (i == 0) continue;
            const auto& left = grid[i - 1];
            const auto& right = grid[i];
            if (left.valid() && left.difference * right.difference < 0) {
                refineBracket(left, right);
                if (stopped()) break;
            }
        }
        // Equal-sign local extrema may represent tangent roots with no sign
        // change. This bounded minimization is deliberately heuristic.
        for (std::size_t i = 1; i + 1 < grid.size() && !stopped(); ++i) {
            const auto& left = grid[i - 1];
            const auto& mid = grid[i];
            const auto& right = grid[i + 1];
            if (!left.valid() || !mid.valid() || !right.valid()) continue;
            if (!((left.difference > 0 && mid.difference > 0 && right.difference > 0) ||
                  (left.difference < 0 && mid.difference < 0 && right.difference < 0))) continue;
            if (std::abs(mid.difference) < std::abs(left.difference) &&
                std::abs(mid.difference) < std::abs(right.difference))
                refineMinimum(left.x, right.x);
        }
        std::sort(result_.points.begin(), result_.points.end(),
                  [](const auto& a, const auto& b) { return a.x < b.x; });
        return result_;
    }

private:
    const CartesianIntersectionFunction& first_;
    const CartesianIntersectionFunction& second_;
    const IntersectionOptions& options_;
    IntersectionResult result_;

    bool stopped() const {
        return result_.status == IntersectionStatus::EvaluationLimit ||
               result_.status == IntersectionStatus::ResultLimit;
    }
    bool probe(double x, Probe& p) {
        if (options_.maxEvaluations - result_.evaluations < 2) return false;
        p.x = x;
        try { p.f = first_(x); } catch (...) { p.f = std::numeric_limits<double>::quiet_NaN(); }
        ++result_.evaluations;
        try { p.g = second_(x); } catch (...) { p.g = std::numeric_limits<double>::quiet_NaN(); }
        ++result_.evaluations;
        if (std::isfinite(p.f) && std::isfinite(p.g)) {
            p.difference = p.f - p.g;
            if (!std::isfinite(p.difference))
                p.difference = std::numeric_limits<double>::quiet_NaN();
        }
        return true;
    }
    void add(const Probe& p, double bracketWidth) {
        if (!p.valid() || std::abs(p.difference) > options_.residualTolerance) {
            ++result_.rejectedCandidates;
            return;
        }
        const double xTolerance = 4 * options_.xTolerance;
        for (auto& existing : result_.points) {
            if (std::abs(existing.x - p.x) <= xTolerance) {
                if (std::abs(p.difference) < existing.residual)
                    existing = {p.x, std::midpoint(p.f, p.g), std::abs(p.difference), bracketWidth};
                return;
            }
        }
        if (result_.points.size() >= options_.maxResults) {
            result_.status = IntersectionStatus::ResultLimit;
            return;
        }
        result_.points.push_back({p.x, std::midpoint(p.f, p.g),
                                   std::abs(p.difference), bracketWidth});
    }
    void refineBracket(Probe left, Probe right) {
        Probe best = std::abs(left.difference) < std::abs(right.difference) ? left : right;
        for (int iteration = 0; iteration < 90; ++iteration) {
            const double width = right.x - left.x;
            if (width <= options_.xTolerance &&
                std::abs(best.difference) <= options_.residualTolerance) break;
            Probe mid;
            const double center = std::midpoint(left.x, right.x);
            if (center == left.x || center == right.x) break;
            if (!probe(center, mid)) {
                result_.status = IntersectionStatus::EvaluationLimit;
                return;
            }
            if (!mid.valid()) { ++result_.rejectedCandidates; return; }
            if (std::abs(mid.difference) < std::abs(best.difference)) best = mid;
            if (mid.difference == 0) { best = mid; left = mid; right = mid; break; }
            if (std::signbit(mid.difference) == std::signbit(left.difference)) left = mid;
            else right = mid;
        }
        add(best, right.x - left.x);
    }
    void refineMinimum(double low, double high) {
        constexpr double factor = 0.3819660112501051518;
        double a = low, b = high;
        Probe p, q;
        if (!probe(a + factor * (b-a), p) || !probe(b - factor * (b-a), q)) {
            result_.status = IntersectionStatus::EvaluationLimit;
            return;
        }
        if (!p.valid() || !q.valid()) { ++result_.rejectedCandidates; return; }
        Probe best = std::abs(p.difference) < std::abs(q.difference) ? p : q;
        for (int iteration = 0; iteration < 80 && (b-a) > options_.xTolerance; ++iteration) {
            if (std::abs(p.difference) <= std::abs(q.difference)) {
                b = q.x;
                q = p;
                if (!probe(a + factor*(b-a), p)) {
                    result_.status = IntersectionStatus::EvaluationLimit;
                    return;
                }
                if (!p.valid()) return;
                if (std::abs(p.difference) < std::abs(best.difference)) best = p;
            } else {
                a = p.x;
                p = q;
                if (!probe(b - factor*(b-a), q)) {
                    result_.status = IntersectionStatus::EvaluationLimit;
                    return;
                }
                if (!q.valid()) return;
                if (std::abs(q.difference) < std::abs(best.difference)) best = q;
            }
        }
        add(best, b-a);
    }
};
} // namespace

IntersectionResult findCartesianIntersections(
    const CartesianIntersectionFunction& first,
    const CartesianIntersectionFunction& second,
    double minimum, double maximum, const IntersectionOptions& options) {
    return Detection(first, second, options).execute(minimum, maximum);
}

IntersectionResult findCartesianIntersections(
    const Expression& first, const Expression& second,
    double minimum, double maximum, AngleMode angle,
    const VariableBindings& bindings, double ans, const IntersectionOptions& options) {
    if (!first.valid() || !second.valid()) return {};
    return findCartesianIntersections(
        [&](double x) { return first.evaluate(x, ans, angle, bindings); },
        [&](double x) { return second.evaluate(x, ans, angle, bindings); },
        minimum, maximum, options);
}
} // namespace rationalstudio
