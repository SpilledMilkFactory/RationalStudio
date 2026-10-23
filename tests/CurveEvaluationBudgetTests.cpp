#include "CurveSampler.hpp"
#include "CurveGeometryCache.hpp"

#include <cmath>
#include <iostream>
#include <memory>
#include <limits>
#include <stdexcept>

using namespace rationalstudio;
namespace {
std::size_t assertions = 0;
void check(bool value, const char* description) {
    if (!value) throw std::runtime_error(description);
    ++assertions;
}
GraphViewport viewport() {
    GraphViewport v;
    check(v.setDimensions(800, 600), "set viewport dimensions");
    check(v.setView(0.0, 0.0, 100.0), "set viewport view");
    return v;
}
void sharedCap() {
    const GraphViewport v = viewport();
    CurveEvaluationBudget shared(31);
    CurveSamplingOptions options;
    options.maxEvaluations = 10000;
    std::size_t invoked = 0;
    const auto curve = [&](double x) { ++invoked; return std::sin(100.0*x); };
    const auto first = sampleExplicitCurve(v, curve, options, &shared);
    check(first.truncated, "first curve truncates under shared budget");
    check(first.truncationReason == CurveTruncationReason::SharedBudget,
          "first curve reports shared budget");
    check(first.evaluations == 31 && invoked == 31 && shared.consumed() == 31,
          "exact shared count without off-by-one");
    const auto second = sampleExplicitCurve(v, curve, options, &shared);
    check(second.truncated && second.evaluations == 0,
          "exhausted budget does not invoke callback");
    check(second.truncationReason == CurveTruncationReason::SharedBudget,
          "zero-work truncation reason");
    check(shared.remaining() == 0 && shared.exhausted() && invoked == 31,
          "shared budget never overdrawn");
    check(!shared.tryConsume(), "cannot consume past limit");
}
void perCurveCap() {
    const GraphViewport v = viewport();
    CurveEvaluationBudget shared(1000);
    CurveSamplingOptions options;
    options.maxEvaluations = 19;
    std::size_t invocations = 0;
    const auto curve = sampleExplicitCurve(v, [&](double x) {
        ++invocations;
        return std::cos(x * 500.0);
    }, options, &shared);
    check(curve.truncated && curve.truncationReason == CurveTruncationReason::PerCurveLimit,
          "per curve limit takes precedence");
    check(invocations == 19 && curve.evaluations == 19 && shared.consumed() == 19,
          "per curve cap exact");
    check(shared.remaining() == 981, "shared unused capacity preserved");
    options.maxEvaluations = 11;
    const auto next = sampleExplicitCurve(v, [](double x){return std::sin(x * 300.0);},
                                          options, &shared);
    check(next.evaluations == 11 && shared.consumed() == 30,
          "next curve draws from the same shared allowance");
}
void callbackFailuresCount() {
    const GraphViewport v = viewport();
    CurveEvaluationBudget shared(23);
    CurveSamplingOptions options;
    options.maxEvaluations = 999;
    std::size_t invoked = 0;
    const auto result = sampleExplicitCurve(v, [&](double x)->double {
        ++invoked;
        if (x > 0.0) throw std::domain_error("undefined");
        return std::numeric_limits<double>::quiet_NaN();
    }, options, &shared);
    check(result.truncated && shared.consumed() == 23 && invoked == 23,
          "non-finite or throwing callbacks also count");
    check(result.vertices.empty(), "invalid domain has no drawn curve");
    const auto before = shared.consumed();
    options.coarseStepPixels = 0.0;
    const auto invalid = sampleExplicitCurve(v, [](double x){return x;}, options, &shared);
    check(!invalid.truncated && invalid.evaluations == 0 && shared.consumed() == before,
          "invalid options never consume budget");
}
void zeroBudgetAndReset() {
    const GraphViewport v = viewport();
    CurveEvaluationBudget empty(0);
    std::size_t invoked = 0;
    const auto result = sampleExplicitCurve(v, [&](double x){++invoked; return x;}, {}, &empty);
    check(result.truncated && result.truncationReason == CurveTruncationReason::SharedBudget,
          "zero shared allowance is explicit truncation");
    check(invoked == 0 && empty.exhausted(), "zero budget does not invoke user code");
    CurveEvaluationBudget fresh(1000);
    const auto complete = sampleExplicitCurve(v, [](double x){return x;}, {}, &fresh);
    check(!complete.truncated && !complete.vertices.empty(),
          "fresh independent budget permits normal curve");
    check(complete.evaluations == fresh.consumed() && fresh.consumed() < 1000,
          "completed curve has exact consumption");
    check(complete.truncationReason == CurveTruncationReason::None,
          "normal sampling has no reason for truncation");
}
void sweepBudgets() {
    const GraphViewport v = viewport();
    for (std::size_t allowance = 0; allowance < 257; ++allowance) {
        CurveEvaluationBudget budget(allowance);
        std::size_t calls = 0;
        CurveSamplingOptions opts;
        opts.maxEvaluations = 37;
        for (int curve = 0; curve < 7; ++curve) {
            const auto samples = sampleExplicitCurve(v, [&](double x) {
                ++calls;
                return std::sin((curve + 1) * 1000.0 * x);
            }, opts, &budget);
            check(samples.evaluations <= opts.maxEvaluations,
                  "each mathematical curve respects its per-call limit");
            check(budget.consumed() <= allowance,
                  "shared count never exceeds initial capacity");
            check(samples.evaluations <= allowance,
                  "individual evaluation count never exceeds shared capacity");
        }
        check(calls == budget.consumed(),
              "shared accounting agrees with actual callback invocations");
        check(budget.remaining() + budget.consumed() == allowance,
              "budget conservation across multiple curves");
    }
}

void cacheStatus() {
    CurveGeometryCache<int> cache;
    const auto key = std::make_shared<Expression>("x");
    cache.synchronize({key});
    bool threw = false;
    try { (void)cache.readyGeometry(0); }
    catch (const std::logic_error&) { threw = true; }
    check(threw, "unbuilt geometry cannot be mistaken for cached empty value");
    check(cache.getOrBuild(0, [] { return 42; }) == 42, "cache stores finished geometry");
    check(cache.readyGeometry(0) == 42, "read-only ready access does not rebuild");
    cache.invalidateAll();
    check(!cache.isReady(0), "invalidation makes geometry dirty");
}
}
int main() {
    try {
        sharedCap(); perCurveCap(); callbackFailuresCount();
        zeroBudgetAndReset(); sweepBudgets(); cacheStatus();
        std::cout << "PASS: " << assertions << " CurveEvaluationBudget assertions\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
