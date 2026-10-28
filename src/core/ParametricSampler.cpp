#include "ParametricSampler.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <utility>

namespace rationalstudio {
namespace {
constexpr unsigned kTiles = 128;
constexpr unsigned kMaxDepth = 10;
constexpr double kScreenLimit = 1e7;
constexpr double kTolerance = 0.8;
struct BudgetYield {};
struct Probe {
    double t = 0, x = 0, y = 0;
    bool valid = false;
};
}

bool validParameterInterval(double low, double high) noexcept {
    return std::isfinite(low) && std::isfinite(high) &&
           low >= -1e8 && high <= 1e8 && low < high &&
           std::isfinite(high - low) && high - low >= 1e-12;
}

struct ParametricSamplingJob::Impl {
    GraphViewport view;
    ParametricFunction fn;
    double low, high;
    unsigned tile = 0;
    std::map<double, Probe> retry;
    CurveSamples output;
    bool connected = false;
    double previousX = 0, previousY = 0;
    std::size_t advanceEnd = 0;
    CurveEvaluationBudget* shared = nullptr;

    Impl(GraphViewport v, ParametricFunction f, double a, double b)
        : view(v), fn(std::move(f)), low(a), high(b) {}

    Probe probe(double t) {
        if (const auto it = retry.find(t); it != retry.end()) return it->second;
        if (output.evaluations >= advanceEnd || !shared || !shared->tryConsume())
            throw BudgetYield{};
        ++output.evaluations;
        Probe probe{t, 0, 0, false};
        const auto world = fn(t);
        if (std::isfinite(world.x) && std::isfinite(world.y)) {
            probe.x = view.worldToScreenX(world.x);
            probe.y = view.worldToScreenY(world.y);
            probe.valid = std::isfinite(probe.x) && std::isfinite(probe.y) &&
                          std::abs(probe.x) <= kScreenLimit &&
                          std::abs(probe.y) <= kScreenLimit;
        }
        retry.emplace(t, probe);
        return probe;
    }
    void disconnect() noexcept { connected = false; }
    void append(const Probe& p) {
        if (!p.valid) { disconnect(); return; }
        if (!connected) {
            output.vertices.push_back({p.x, p.y, true});
            connected = true;
        } else if (p.x != previousX || p.y != previousY) {
            output.vertices.push_back({p.x, p.y, false});
        }
        previousX = p.x; previousY = p.y;
    }
    void refine(const Probe& a, const Probe& b, unsigned depth) {
        // Always inspect the midpoint before joining endpoints: a finite
        // endpoint pair alone does not prove a continuous parameter interval.
        const double middle = std::midpoint(a.t, b.t);
        if (middle == a.t || middle == b.t) {
            if (a.valid && b.valid) { append(a); append(b); }
            else disconnect();
            return;
        }
        const auto m = probe(middle);
        if (a.valid && b.valid && m.valid) {
            const double chord = std::hypot(b.x - a.x, b.y - a.y);
            const double error = std::hypot(m.x - (a.x + b.x)*0.5,
                                            m.y - (a.y + b.y)*0.5);
            // Reject very long terminal chords, even if nearly collinear;
            // they often bridge undefined poles or finite jump boundaries.
            const double terminalLimit = 2.0 * std::hypot(view.width(), view.height());
            if (error <= kTolerance && chord <= 36.0) {
                append(a); append(b);
                return;
            }
            if (depth >= kMaxDepth) {
                if (error <= kTolerance && chord <= terminalLimit) {
                    append(a); append(b);
                } else disconnect();
                return;
            }
        } else if (depth >= kMaxDepth) {
            disconnect();
            return;
        }
        refine(a, m, depth + 1);
        refine(m, b, depth + 1);
    }
    bool advance(std::size_t allowance, CurveEvaluationBudget& budget) {
        if (!fn || !view.isValid() || !validParameterInterval(low, high)) return true;
        shared = &budget;
        advanceEnd = output.evaluations + allowance;
        while (tile < kTiles) {
            const auto savedSize = output.vertices.size();
            const auto savedConnected = connected;
            const double savedX = previousX, savedY = previousY;
            const auto begin = std::lerp(low, high, double(tile) / kTiles);
            const auto end = std::lerp(low, high, double(tile + 1) / kTiles);
            try {
                const auto a = probe(begin);
                const auto b = probe(end);
                refine(a, b, 0);
            } catch (const BudgetYield&) {
                output.vertices.resize(savedSize);
                connected = savedConnected;
                previousX = savedX; previousY = savedY;
                return false;
            }
            retry.clear();
            ++tile;
        }
        return true;
    }
};

ParametricSamplingJob::ParametricSamplingJob(GraphViewport v, ParametricFunction f,
                                             double a, double b)
    : impl_(std::make_unique<Impl>(v, std::move(f), a, b)) {}
ParametricSamplingJob::~ParametricSamplingJob() = default;
bool ParametricSamplingJob::advance(std::size_t n, CurveEvaluationBudget& budget) {
    return impl_->advance(n, budget);
}
const CurveSamples& ParametricSamplingJob::samples() const { return impl_->output; }

CurveSamples sampleParametricCurve(const GraphViewport& view, const ParametricFunction& fn,
                                   double low, double high, std::size_t limit,
                                   CurveEvaluationBudget* shared) {
    ParametricSamplingJob job(view, fn, low, high);
    CurveEvaluationBudget local(limit);
    auto& budget = shared ? *shared : local;
    const auto allowed = std::min(limit, budget.remaining());
    const bool completed = job.advance(allowed, budget);
    CurveSamples result = job.samples();
    if (!completed) {
        result.truncated = true;
        result.truncationReason = shared && shared->exhausted()
            ? CurveTruncationReason::SharedBudget : CurveTruncationReason::PerCurveLimit;
    }
    return result;
}
} // namespace rationalstudio
