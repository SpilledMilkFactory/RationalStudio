#include "CurveSampler.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <vector>

using namespace rationalstudio;
namespace {
std::size_t assertions = 0;
void require(bool condition, const char* message) {
    ++assertions;
    if (!condition) throw std::runtime_error(message);
}
CurveSamples complete(GraphViewport view, const std::function<double(double)>& function,
                      std::size_t allowance = 8192) {
    std::size_t calls = 0;
    CurveSamplingJob job(view, [&](double x) { ++calls; return function(x); });
    bool done = false;
    for (int frame = 0; frame < 10000 && !done; ++frame) {
        CurveEvaluationBudget shared(allowance);
        const auto before = calls;
        done = job.advance(allowance, shared);
        require(calls - before == shared.consumed(), "only new probes consume callbacks");
        require(shared.consumed() <= allowance, "hard allowance per advance");
    }
    require(done, "unfinished sampling eventually completes");
    require(job.samples().evaluations == calls, "cumulative callback count");
    CurveEvaluationBudget cached(8192);
    require(job.advance(8192, cached) && cached.consumed() == 0,
            "completed geometry incurs no additional probes");
    return job.samples();
}

void tangentBranches() {
    for (int width : {350, 900, 1600}) {
        for (double scale : {58.0, 15.0, 1.38, 0.5, 0.1}) {
            GraphViewport view;
            require(view.setDimensions(width, 617), "viewport dimensions");
            require(view.setView(13.7, 37.0, scale), "translated viewport");
            const auto samples = complete(view, [](double x) { return std::tan(x); });
            std::vector<double> top, bottom;
            CurveVertex previous{};
            for (const auto& point : samples.vertices) {
                require(std::isfinite(point.screenX) && std::isfinite(point.screenY),
                        "finite drawing coordinates");
                require(point.screenY >= -2.0 && point.screenY <= 619.0,
                        "geometry clipped before reaching rasterizer");
                if (!point.moveTo) {
                    require(point.screenX > previous.screenX, "ordered, nonduplicated segments");
                    const double a = view.screenToWorldX(previous.screenX);
                    const double b = view.screenToWorldX(point.screenX);
                    const double nextPole = std::numbers::pi *
                        (std::floor(a / std::numbers::pi - 0.5) + 1.5);
                    require(!(a < nextPole && b >= nextPole), "no chord bridges a tangent pole");
                    for (auto edge : {0.0, 617.0}) {
                        if (std::min(previous.screenY, point.screenY) <= edge &&
                            std::max(previous.screenY, point.screenY) >= edge &&
                            point.screenY != previous.screenY) {
                            const double t = (edge-previous.screenY)/(point.screenY-previous.screenY);
                            (edge == 0.0 ? top : bottom).push_back(
                                std::lerp(previous.screenX, point.screenX, t));
                        }
                    }
                }
                previous = point;
            }
            // Every analytically known intersection with both viewport edges
            // must be present, rather than merely one vertex in each x bin.
            for (auto edge : {0.0, 617.0}) {
                const auto& crossings = edge == 0.0 ? top : bottom;
                const double phase = std::atan(view.screenToWorldY(edge));
                const int first = static_cast<int>(std::floor(
                    (view.screenToWorldX(0)-phase)/std::numbers::pi));
                const int last = static_cast<int>(std::ceil(
                    (view.screenToWorldX(width)-phase)/std::numbers::pi));
                for (int k = first; k <= last; ++k) {
                    const double expected = view.worldToScreenX(phase + k*std::numbers::pi);
                    if (expected < 2 || expected > width-2) continue;
                    const bool found = std::any_of(crossings.begin(), crossings.end(), [&](double x) {
                        return std::abs(x-expected) <= 1.0;
                    });
                    if (!found) std::cerr << "missing edge: width=" << width << " scale=" << scale
                                         << " edge=" << edge << " x=" << expected << '\n';
                    require(found, "every tangent branch reaches both viewport edges");
                }
            }
        }
    }
}

void discontinuityControls() {
    GraphViewport view;
    require(view.setDimensions(800, 600), "control dimensions");
    require(view.setView(0, 0, 100), "control view");
    for (double edge : {0.0, 0.137, -0.327}) {
        for (bool reciprocal : {false, true}) {
            const auto samples = complete(view, [=](double x) {
                return reciprocal ? 1.0/(x-edge) : (x < edge ? -1.0 : 1.0);
            }, 31);
            CurveVertex previous{};
            bool left = false, right = false;
            for (const auto& point : samples.vertices) {
                if (!point.moveTo) {
                    const double a = view.screenToWorldX(previous.screenX);
                    const double b = view.screenToWorldX(point.screenX);
                    require(!(a < edge && b >= edge), "incremental job preserves pole and step gaps");
                    left = left || b < edge;
                    right = right || a > edge;
                }
                previous = point;
            }
            require(left && right, "both discontinuity branches survive yielding");
        }
        const auto smooth = complete(view, [=](double x) {
            return std::tanh(1000*(x-edge));
        }, 31);
        CurveVertex previous{};
        bool connected = false;
        for (const auto& point : smooth.vertices) {
            if (!point.moveTo && view.screenToWorldX(previous.screenX) < edge &&
                view.screenToWorldX(point.screenX) >= edge) connected = true;
            previous = point;
        }
        require(connected, "steep continuous transitions remain connected");
    }
}

void budgetIndependentGeometry() {
    GraphViewport view;
    require(view.setDimensions(350, 300), "small dimensions");
    require(view.setView(0.0, 0.0, 15), "small view");
    const auto curve = [](double x) { return std::tan(x); };
    const auto tiny = complete(view, curve, 4);
    const auto large = complete(view, curve, 65536);
    require(tiny.vertices.size() == large.vertices.size(), "budget does not drop or duplicate geometry");
    for (std::size_t i = 0; i < tiny.vertices.size(); ++i) {
        require(tiny.vertices[i].screenX == large.vertices[i].screenX &&
                tiny.vertices[i].screenY == large.vertices[i].screenY &&
                tiny.vertices[i].moveTo == large.vertices[i].moveTo,
                "yielding preserves exact deterministic geometry");
    }
}
}
int main() {
    try {
        tangentBranches();
        budgetIndependentGeometry();
        discontinuityControls();
        std::cout << "PASS: " << assertions << " sampling job assertions\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
