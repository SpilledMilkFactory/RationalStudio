#include "NumericalIntegration.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <queue>
#include <vector>

namespace rationalstudio {
    namespace {

        // Positive Gauss-Kronrod 15-point nodes and weights (QUADPACK qk15).
        // The shared Gauss-7 nodes are indices 1, 3, and 5 plus the center.
        constexpr std::array<double, 8> xgk{
            0.991455371120812639206854697526329,
            0.949107912342758524526189684047851,
            0.864864423359769072789712788640926,
            0.741531185599394439863864773280788,
            0.586087235467691130294144838258730,
            0.405845151377397166906606412076961,
            0.207784955007898467600689403773245,
            0.0};
            constexpr std::array<double, 8> wgk{
                0.022935322010529224963732008058970,
                0.063092092629978553290700663189204,
                0.104790010322250183839876322541518,
                0.140653259715525918745189590510238,
                0.169004726639267902826583426598550,
                0.190350578064785409913256412076961,
                0.204432940075298892414161999234649,
                0.209482141084727828012999174891714};
                constexpr std::array<double, 4> wg{
                    0.129484966168869693270611432679082,
                    0.279705391489276667901467771423780,
                    0.381830050505118944950369775488975,
                    0.417959183673469387755102040816327};

                    bool valid(const IntegrationFunction& function, double a, double b,
                               const IntegrationOptions& o) noexcept {
                                   return static_cast<bool>(function) && std::isfinite(a) && std::isfinite(b)
                                   && std::isfinite(o.absoluteTolerance) && o.absoluteTolerance > 0
                                   && std::isfinite(o.relativeTolerance) && o.relativeTolerance > 0
                                   && o.maxEvaluations >= 15 && o.maxEvaluations <= 10000000
                                   && o.maxSubdivisions >= 1 && o.maxSubdivisions <= 100000
                                   && o.maxDepth >= 1 && o.maxDepth <= 128;
                               }

                               struct Panel {
                                   double a = 0;
                                   double b = 0;
                                   double value = 0;
                                   double error = 0;
                                   std::size_t depth = 0;
                                   std::size_t id = 0;
                               };

                               // Max-heap: largest numerical error is bisected first, with deterministic ties.
                               struct LargerError {
                                   bool operator()(const Panel& lhs, const Panel& rhs) const noexcept {
                                       if (lhs.error == rhs.error) return lhs.id > rhs.id;
                                       return lhs.error < rhs.error;
                                   }
                               };

                               struct State {
                                   const IntegrationFunction& function;
                                   std::size_t limit;
                                   std::size_t evaluations = 0;
                                   bool exhausted = false;
                                   bool invalid = false;

                                   double sample(double x) {
                                       if (evaluations >= limit) {
                                           exhausted = true;
                                           return std::numeric_limits<double>::quiet_NaN();
                                       }
                                       ++evaluations;
                                       double y = std::numeric_limits<double>::quiet_NaN();
                                       try { y = function(x); } catch (...) { /* domain/callback failure */ }
                                       if (!std::isfinite(y)) invalid = true;
                                       return y;
                                   }

                                   bool evaluate(Panel& panel) {
                                       const double half = (panel.b - panel.a) * 0.5;
                                       const double center = panel.a + half;
                                       // Reject degenerate/unrepresentable intervals; otherwise nodes collapse.
                                       if (!(half > 0) || !std::isfinite(half) ||
                                           center == panel.a || center == panel.b) return false;
                                       if (limit - evaluations < 15) { exhausted = true; return false; }

                                       std::array<double, 7> left{};
                                       std::array<double, 7> right{};
                                       const double fc = sample(center);
                                       if (invalid) return false;
                                       long double resg = wg[3] * static_cast<long double>(fc);
                                       long double resk = wgk[7] * static_cast<long double>(fc);
                                       long double resabs = wgk[7] * std::abs(static_cast<long double>(fc));
                                       for (std::size_t j = 0; j < 7; ++j) {
                                           // The symmetric points remain inside the representable interval.
                                           const double delta = half * xgk[j];
                                           const double xleft = center - delta, xright = center + delta;
                                           if (!(xleft >= panel.a && xright <= panel.b) ||
                                               xleft == xright) return false;
                                           left[j] = sample(xleft);
                                           right[j] = sample(xright);
                                           if (invalid) return false;
                                           const long double pair = static_cast<long double>(left[j]) + right[j];
                                           resk += wgk[j] * pair;
                                           resabs += wgk[j] * (std::abs(static_cast<long double>(left[j]))
                                           + std::abs(static_cast<long double>(right[j])));
                                           if (j == 1 || j == 3 || j == 5)
                                               resg += wg[(j - 1) / 2] * pair;
                                       }
                                       const long double mean = resk * 0.5L;
                                       long double resasc = wgk[7] * std::abs(static_cast<long double>(fc) - mean);
                                       for (std::size_t j = 0; j < 7; ++j)
                                           resasc += wgk[j] * (std::abs(static_cast<long double>(left[j]) - mean)
                                           + std::abs(static_cast<long double>(right[j]) - mean));
                                       const long double area = resk * half;
                                       long double err = std::abs((resk - resg) * half);
                                       const long double asc = resasc * half;
                                       const long double absarea = resabs * half;
                                       if (asc > 0 && err > 0)
                                           err = asc * std::min(1.0L, std::pow(200.0L * err / asc, 1.5L));
                                       // QUADPACK-style roundoff floor; avoids claiming arbitrarily tiny errors.
                                       const long double floor = 50 * std::numeric_limits<double>::epsilon() * absarea;
                                       err = std::max(err, floor);
                                       if (!std::isfinite(area) || !std::isfinite(err) ||
                                           std::abs(area) > std::numeric_limits<double>::max() ||
                                           err > std::numeric_limits<double>::max()) {
                                           invalid = true;
                                       return false;
                                           }
                                           panel.value = static_cast<double>(area);
                                           panel.error = static_cast<double>(err);
                                           return true;
                                   }
                               };

    } // namespace

    IntegrationResult integrate(const IntegrationFunction& function, double lower,
                                double upper, const IntegrationOptions& options) {
        IntegrationResult out;
        if (!valid(function, lower, upper, options)) return out;
        if (lower == upper) {
            out.status = IntegrationStatus::Converged;
            out.value = 0;
            out.estimatedError = 0;
            return out;
        }
        const double sign = upper < lower ? -1.0 : 1.0;
        const double a = std::min(lower, upper), b = std::max(lower, upper);
        if (!std::isfinite(b - a)) return out;
        State state{function, options.maxEvaluations};
        Panel initial{a, b, 0, 0, 0, 0};
        auto fail = [&](IntegrationStatus reason, long double error,
                        std::size_t intervals) {
            out.status = reason;
            out.evaluations = state.evaluations;
            out.intervals = intervals;
            if (std::isfinite(error) && error <= std::numeric_limits<double>::max())
                out.estimatedError = static_cast<double>(error);
            return out;
                        };
                        if (!state.evaluate(initial))
                            return fail(state.exhausted ? IntegrationStatus::EvaluationLimit
                            : state.invalid ? IntegrationStatus::UndefinedIntegrand
                            : IntegrationStatus::Unresolved,
                            std::numeric_limits<long double>::infinity(), 0);

                        std::priority_queue<Panel, std::vector<Panel>, LargerError> queue;
                        queue.push(initial);
                        long double total = initial.value, totalError = initial.error;
                        std::size_t nextId = 1;
                        while (true) {
                            const long double tolerance = std::max(
                                static_cast<long double>(options.absoluteTolerance),
                                                                   static_cast<long double>(options.relativeTolerance) * std::abs(total));
                            if (totalError <= tolerance) {
                                if (!std::isfinite(total) ||
                                    std::abs(total) > std::numeric_limits<double>::max())
                                    return fail(IntegrationStatus::Unresolved, totalError, queue.size());
                                out.status = IntegrationStatus::Converged;
                                out.value = sign * static_cast<double>(total);
                                out.estimatedError = static_cast<double>(totalError);
                                out.evaluations = state.evaluations;
                                out.intervals = queue.size();
                                return out;
                            }
                            if (out.subdivisions >= options.maxSubdivisions)
                                return fail(IntegrationStatus::SubdivisionLimit, totalError, queue.size());
                            if (options.maxEvaluations - state.evaluations < 30)
                                return fail(IntegrationStatus::EvaluationLimit, totalError, queue.size());
                            const Panel parent = queue.top();
                            if (parent.depth >= options.maxDepth)
                                return fail(IntegrationStatus::Unresolved, totalError, queue.size());
                            const double midpoint = parent.a + (parent.b - parent.a) * 0.5;
                            if (midpoint == parent.a || midpoint == parent.b)
                                return fail(IntegrationStatus::Unresolved, totalError, queue.size());
                            Panel left{parent.a, midpoint, 0, 0, parent.depth + 1, nextId++};
                            Panel right{midpoint, parent.b, 0, 0, parent.depth + 1, nextId++};
                            // Publish both child panels atomically, never a half-refined partition.
                            if (!state.evaluate(left) || !state.evaluate(right))
                                return fail(state.exhausted ? IntegrationStatus::EvaluationLimit
                                : state.invalid ? IntegrationStatus::UndefinedIntegrand
                                : IntegrationStatus::Unresolved,
                                totalError, queue.size());
                            queue.pop();
                            queue.push(left);
                            queue.push(right);
                            total += static_cast<long double>(left.value) + right.value - parent.value;
                            totalError += static_cast<long double>(left.error) + right.error - parent.error;
                            totalError = std::max(0.0L, totalError);
                            ++out.subdivisions;
                        }
                                }

                                IntegrationResult integrate(const Expression& expression, double lower,
                                                            double upper, AngleMode angle,
                                                            const VariableBindings& bindings, double ans,
                                                            const IntegrationOptions& options) {
                                    if (!expression.valid() || !std::isfinite(ans)) return {};
                                    const VariableBindings snapshot = bindings;
                                    return integrate([&](double x) {
                                        return expression.evaluate(x, ans, angle, snapshot);
                                    }, lower, upper, options);
                                                            }

                                                            const char* integrationStatusName(IntegrationStatus status) noexcept {
                                                                switch (status) {
                                                                    case IntegrationStatus::Converged: return "converged";
                                                                    case IntegrationStatus::InvalidInput: return "invalid input";
                                                                    case IntegrationStatus::UndefinedIntegrand: return "undefined integrand";
                                                                    case IntegrationStatus::EvaluationLimit: return "evaluation limit";
                                                                    case IntegrationStatus::SubdivisionLimit: return "subdivision limit";
                                                                    case IntegrationStatus::Unresolved: return "unresolved or roundoff limited";
                                                                }
                                                                return "unknown";
                                                            }

                                                            const char* integrationMethodName(IntegrationMethod method) noexcept {
                                                                switch (method) {
                                                                    case IntegrationMethod::GaussKronrod15: return "adaptive Gauss-Kronrod 7/15";
                                                                }
                                                                return "unknown";
                                                            }

} // namespace rationalstudio
