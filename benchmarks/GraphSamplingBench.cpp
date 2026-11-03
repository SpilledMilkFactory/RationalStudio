#include "BenchmarkMetrics.hpp"
#include "CurveSampler.hpp"
#include "GraphViewport.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace rationalstudio;
using namespace rationalstudio::benchmark;
namespace {
constexpr std::size_t kCurveBudget = 8192;
constexpr std::size_t kPaintBudget = 32768;
std::vector<std::function<double(double)>> functions() {
    return {
        [](double x) { return std::sin(x); },
        [](double x) { return 0.08*x*x - 2.0; },
        [](double x) { return std::cos(x*3.0); },
        [](double x) { return std::tanh(1000.0*(x - 0.137)); },
        [](double x) { return 1.0/(x - 0.133); },
        [](double x) { return std::floor(x); },
        [](double x) { return std::atan(120.0*x); },
        [](double x) { return std::sqrt(x); }
    };
}
void sampleScenario(const std::vector<std::function<double(double)>>& curves,
                    std::size_t count, GraphViewport view,
                    std::vector<Measurement>& rows, const std::string& scenario,
                    int iteration, bool record) {
    CurveEvaluationBudget shared(kPaintBudget);
    std::size_t vertices = 0, built = 0;
    bool truncated = false;
    std::size_t dirty = count;
    const auto start = Clock::now();
    for (std::size_t index = 0; index < count; ++index) {
        if (shared.remaining() < 4) { truncated = true; break; }
        const std::size_t fairShare = dirty ? shared.remaining() / dirty : shared.remaining();
        const std::size_t quota = std::min({kCurveBudget, shared.remaining(),
                                           std::max<std::size_t>(1024, fairShare)});
        CurveSamplingOptions options;
        options.maxEvaluations = quota;
        const auto samples = sampleExplicitCurve(view, curves[index], options, &shared);
        vertices += samples.vertices.size();
        truncated |= samples.truncated;
        ++built;
        --dirty;
    }
    const auto stop = Clock::now();
    if (shared.consumed() > kPaintBudget)
        throw std::runtime_error("sampling benchmark violated shared evaluation cap");
    if (record) rows.push_back({"sampling_callbacks_plus_geometry", scenario, count,
                                static_cast<std::size_t>(iteration),
                                milliseconds(start, stop), shared.consumed(), built,
                                vertices, truncated || built < count});
}
}
int main(int argc, char** argv) {
    try {
        const auto options = parseOptions(argc, argv);
        const auto curves = functions();
        std::vector<Measurement> measurements;
        for (const auto& [name, scale] : {std::pair{"default_view", 58.0},
                                          std::pair{"zoomed_view", 850.0}}) {
            GraphViewport view;
            if (!view.setDimensions(960, 600) || !view.setView(0, 0, scale))
                throw std::runtime_error("invalid benchmark viewport");
            for (std::size_t count : {std::size_t{1}, std::size_t{4}, std::size_t{8}}) {
                for (int i = -options.warmups; i < options.repetitions; ++i)
                    sampleScenario(curves, count, view, measurements, name, i,
                                   i >= 0);
            }
        }
        writeMeasurements(options.output, "graph_sampling", measurements,
                          "960x600 viewport; 1/4/8 active curves; max 8192 callbacks per curve "
                          "and 32768 total per pass. Timed region includes callback evaluation "
                          "and vector assembly, but excludes Qt rendering, AST parsing, and I/O. "
                          "Cold pass performed each iteration; not a frame-time guarantee.");
        std::cout << "Graph sampling benchmark complete: " << measurements.size()
                  << " samples; CSV/JSON in " << options.output << '\n';
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Graph sampling benchmark: " << ex.what() << '\n';
        return 1;
    }
}
