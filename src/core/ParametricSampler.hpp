#pragma once

#include "CurveSampler.hpp"
#include "GraphViewport.hpp"
#include <functional>
#include <memory>

namespace rationalstudio {

// A shared screen-space geometry contract: sampled parametric vertices use
// CurveVertex and CurveSamples just like Cartesian geometry. No Qt dependency.
struct ParametricPoint { double x = 0.0; double y = 0.0; };
using ParametricFunction = std::function<ParametricPoint(double)>;

// Bounded finite interval; no silent clipping of potentially meaningful input.
[[nodiscard]] bool validParameterInterval(double minimum, double maximum) noexcept;

class ParametricSamplingJob {
public:
    ParametricSamplingJob(GraphViewport viewport, ParametricFunction function,
                          double minimum, double maximum);
    ~ParametricSamplingJob();
    ParametricSamplingJob(const ParametricSamplingJob&) = delete;
    ParametricSamplingJob& operator=(const ParametricSamplingJob&) = delete;
    // Returns true when every segment has been considered; false on budget yield.
    bool advance(std::size_t allowance, CurveEvaluationBudget& sharedBudget);
    [[nodiscard]] const CurveSamples& samples() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Single-call bounded version for Qt-independent tests and future backends.
[[nodiscard]] CurveSamples sampleParametricCurve(
    const GraphViewport& viewport, const ParametricFunction& function,
    double minimum, double maximum, std::size_t maxEvaluations = 65536,
    CurveEvaluationBudget* sharedBudget = nullptr);

} // namespace rationalstudio
