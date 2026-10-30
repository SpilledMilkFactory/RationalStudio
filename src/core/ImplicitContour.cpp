#include "ImplicitContour.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

namespace rationalstudio {
namespace {
std::string trim(std::string_view input) {
    const auto start = input.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return {};
    const auto end = input.find_last_not_of(" \t\r\n");
    return std::string(input.substr(start, end - start + 1));
}
struct Point { double x, y; };
// Edges in clockwise order: top, right, bottom, left.
constexpr int edges[4][2] = {{0,1}, {1,2}, {2,3}, {3,0}};
}

ImplicitEquationSource implicitEquationSource(std::string_view equation) {
    const auto equal = equation.find('=');
    if (equal == std::string_view::npos || equation.find('=', equal + 1) != std::string_view::npos)
        return {{}, "Implicit equation requires exactly one '=' sign."};
    const std::string lhs = trim(equation.substr(0, equal));
    const std::string rhs = trim(equation.substr(equal + 1));
    if (lhs.empty() || rhs.empty()) return {{}, "Both sides of the implicit equation are required."};
    if (lhs.size() > 2048 || rhs.size() > 2048)
        return {{}, "Implicit equation exceeds expression length limits."};
    return {"(" + lhs + ")-(" + rhs + ")", {}};
}

struct ImplicitContourJob::Impl {
    GraphViewport view;
    ImplicitFunction fn;
    CurveSamples result;
    int columns = 0;
    int rows = 0;
    int column = 0;
    int row = 0;
    double dx = 0;
    double dy = 0;
    bool usable = false;

    Impl(GraphViewport v, ImplicitFunction callback, double step) : view(v), fn(std::move(callback)) {
        if (!view.isValid() || !fn || !std::isfinite(step) || step < 2.0 || step > 128.0)
            return;
        const double across = std::ceil(double(view.width()) / step);
        const double down = std::ceil(double(view.height()) / step);
        if (across < 1 || down < 1 || across * down > 250000.0) return;
        columns = static_cast<int>(across);
        rows = static_cast<int>(down);
        dx = double(view.width()) / columns;
        dy = double(view.height()) / rows;
        usable = true;
    }
    void segment(Point first, Point second) {
        if (!std::isfinite(first.x) || !std::isfinite(first.y) ||
            !std::isfinite(second.x) || !std::isfinite(second.y)) return;
        const double len = std::hypot(second.x - first.x, second.y - first.y);
        if (len <= 1e-9) return;
        result.vertices.push_back({first.x, first.y, true});
        result.vertices.push_back({second.x, second.y, false});
    }
    void cell() {
        const double x0 = column * dx, x1 = (column + 1) * dx;
        const double y0 = row * dy, y1 = (row + 1) * dy;
        const std::array<Point,4> corners = {{{x0,y0},{x1,y0},{x1,y1},{x0,y1}}};
        std::array<double,4> f{};
        for (int i = 0; i < 4; ++i) {
            const double wx = view.screenToWorldX(corners[i].x);
            const double wy = view.screenToWorldY(corners[i].y);
            f[i] = std::isfinite(wx) && std::isfinite(wy)
                ? fn(wx, wy) : std::numeric_limits<double>::quiet_NaN();
        }
        result.evaluations += 4;
        if (!std::all_of(f.begin(), f.end(), [](double v) { return std::isfinite(v); })) return;
        std::array<Point,4> intersections{};
        std::array<int,4> crossed{};
        int count = 0;
        for (int e = 0; e < 4; ++e) {
            const int a = edges[e][0], b = edges[e][1];
            if ((f[a] < 0) == (f[b] < 0)) continue;
            // Stable linear interpolation within the sampled cell. Do not
            // overflow by forming f[a]-f[b] when both are huge and finite.
            const double scale = std::max(std::abs(f[a]), std::abs(f[b]));
            if (!(scale > 0)) continue;
            const double aa = f[a] / scale, bb = f[b] / scale;
            const double denominator = aa - bb;
            if (denominator == 0) continue;
            const double t = std::clamp(aa / denominator, 0.0, 1.0);
            intersections[e] = {
                std::lerp(corners[a].x, corners[b].x, t),
                std::lerp(corners[a].y, corners[b].y, t)
            };
            crossed[count++] = e;
        }
        if (count == 2) {
            segment(intersections[crossed[0]], intersections[crossed[1]]);
        } else if (count == 4) {
            // Ambiguous saddle: classify the bilinear center. The exact
            // tie-breaker is deterministic. We never invent an unbounded
            // bridge across the cell or connect unrelated neighboring cells.
            const double scale = std::max({std::abs(f[0]),std::abs(f[1]),std::abs(f[2]),std::abs(f[3])});
            const double center = (f[0]/scale + f[1]/scale + f[2]/scale + f[3]/scale);
            const bool centerMatches0 = (center < 0) == (f[0] < 0);
            if (centerMatches0) {
                segment(intersections[0], intersections[1]);
                segment(intersections[2], intersections[3]);
            } else {
                segment(intersections[0], intersections[3]);
                segment(intersections[1], intersections[2]);
            }
        }
    }
};

ImplicitContourJob::ImplicitContourJob(GraphViewport view, ImplicitFunction fn, double step)
    : impl_(std::make_unique<Impl>(view, std::move(fn), step)) {}
ImplicitContourJob::~ImplicitContourJob() = default;
bool ImplicitContourJob::valid() const { return impl_->usable; }
const CurveSamples& ImplicitContourJob::samples() const { return impl_->result; }
bool ImplicitContourJob::advance(std::size_t allowance, CurveEvaluationBudget& sharedBudget) {
    if (!impl_->usable) return true; // Rejected settings never call the callback.
    std::size_t used = 0;
    while (impl_->row < impl_->rows) {
        if (allowance - used < 4 || sharedBudget.remaining() < 4) return false;
        for (int i = 0; i < 4; ++i) (void)sharedBudget.tryConsume();
        used += 4;
        impl_->cell();
        if (++impl_->column == impl_->columns) { impl_->column = 0; ++impl_->row; }
    }
    return true;
}
CurveSamples sampleImplicitContour(const GraphViewport& view, const ImplicitFunction& fn,
    double step, std::size_t maximum, CurveEvaluationBudget* shared) {
    ImplicitContourJob job(view, fn, step);
    CurveEvaluationBudget own(maximum);
    auto& budget = shared ? *shared : own;
    const bool done = job.advance(maximum, budget);
    CurveSamples out = job.samples();
    out.truncated = !done;
    out.truncationReason = done ? CurveTruncationReason::None :
        (budget.remaining() < 4 ? CurveTruncationReason::SharedBudget :
                                     CurveTruncationReason::PerCurveLimit);
    return out;
}
} // namespace rationalstudio
