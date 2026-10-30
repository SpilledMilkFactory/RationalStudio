#pragma once

#include "CurveSampler.hpp"
#include "GraphViewport.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace rationalstudio {

// Presentation-only implicit equation: LHS = RHS becomes (LHS)-(RHS).
// Never adds or modifies names in the global variable/function owner.
struct ImplicitEquationSource {
    std::string expression;
    std::string error;
    [[nodiscard]] bool valid() const noexcept { return error.empty(); }
};
[[nodiscard]] ImplicitEquationSource implicitEquationSource(std::string_view equation);

using ImplicitFunction = std::function<double(double, double)>;

// An incremental marching-squares job. All vertices use the existing
// CurveVertex screen-space contract; each separate segment starts with moveTo.
// The viewport and callback snapshot are immutable for the job lifetime.
class ImplicitContourJob {
public:
    ImplicitContourJob(GraphViewport view, ImplicitFunction function,
                       double gridStepPixels = 12.0);
    ~ImplicitContourJob();
    ImplicitContourJob(const ImplicitContourJob&) = delete;
    ImplicitContourJob& operator=(const ImplicitContourJob&) = delete;

    // Returns false on budget yield. Completion is only true after all cells
    // have been considered; callers must not publish partial geometry.
    bool advance(std::size_t allowance, CurveEvaluationBudget& sharedBudget);
    [[nodiscard]] const CurveSamples& samples() const;
    [[nodiscard]] bool valid() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] CurveSamples sampleImplicitContour(
    const GraphViewport& view, const ImplicitFunction& function,
    double gridStepPixels = 12.0, std::size_t maxEvaluations = 65536,
    CurveEvaluationBudget* sharedBudget = nullptr);

} // namespace rationalstudio
