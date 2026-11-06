// Issue #45: advisory, repeatable Qt-independent advanced graph benchmarks.
// No performance timing assertion is registered with CTest.
#include "CurveSampler.hpp"
#include "ParametricSampler.hpp"
#include "ImplicitContour.hpp"
#include "InequalityRegion.hpp"
#include "GraphTrace.hpp"
#include "PolarExpression.hpp"
#include "Expression.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

using namespace rationalstudio;
namespace {
constexpr double pi=3.1415926535897932384626433832795;
struct Stats { std::size_t evaluations=0, vertices=0; };
using clock_type=std::chrono::steady_clock;
std::size_t observed=0;
template<class Fn>
void report(const std::string& label,int repetitions,Fn&& run) {
    for(int i=0;i<3;++i) { const Stats s=run(); observed+=s.vertices+s.evaluations; }
    std::vector<double> milliseconds;
    std::vector<std::size_t> evaluations,vertices;
    milliseconds.reserve(static_cast<std::size_t>(repetitions));
    for(int i=0;i<repetitions;++i) {
        const auto start=clock_type::now();
        const Stats s=run();
        const auto stop=clock_type::now();
        milliseconds.push_back(std::chrono::duration<double,std::milli>(stop-start).count());
        evaluations.push_back(s.evaluations);
        vertices.push_back(s.vertices);
        observed+=s.evaluations+s.vertices;
    }
    std::sort(milliseconds.begin(),milliseconds.end());
    std::sort(evaluations.begin(),evaluations.end());
    std::sort(vertices.begin(),vertices.end());
    const auto n=milliseconds.size();
    // Nearest-rank p95, intentionally not an asymptotic latency guarantee.
    const auto p95=static_cast<std::size_t>(std::ceil(0.95*static_cast<double>(n)))-1;
    std::cout<<label<<','<<repetitions<<','<<std::fixed<<std::setprecision(6)
             <<milliseconds[n/2]<<','<<milliseconds[p95]<<','<<milliseconds.back()
             <<','<<evaluations[n/2]<<','<<vertices[n/2]<<'\n';
}
}
int main(int argc,char** argv) {
    int repetitions=15;
    if (argc==2) {
        try { repetitions=std::stoi(argv[1]); } catch (...) { std::cerr<<"Invalid repetitions\n";return 2; }
        if (repetitions<5||repetitions>250) { std::cerr<<"repetitions must be 5..250\n";return 2; }
    } else if(argc!=1) {std::cerr<<"Usage: RationalAdvancedGraphBench [repetitions=15]\n";return 2;}
    GraphViewport view;
    if(!view.setDimensions(960,640)||!view.setView(0,0,55))return 2;
    std::cout<<"case,repetitions,median_ms,p95_ms,max_ms,median_evaluations,median_vertices\n";
    report("cartesian_sine",repetitions,[&] {
        auto s=sampleExplicitCurve(view,[](double x){return std::sin(4*x)+0.2*std::cos(11*x);});
        return Stats{s.evaluations,s.vertices.size()};
    });
    report("parametric_circle",repetitions,[&] {
        auto s=sampleParametricCurve(view,[](double t){return ParametricPoint{3*std::cos(t),3*std::sin(t)};},0,2*pi);
        return Stats{s.evaluations,s.vertices.size()};
    });
    const auto rose=polarCoordinateSources("r=2cos(3theta)");
    VariableScope scope;
    if(!scope.declare("t"))return 2;
    Expression px(rose.x,scope),py(rose.y,scope);
    if(!px.valid()||!py.valid())return 2;
    report("polar_rose",repetitions,[&] {
        auto s=sampleParametricCurve(view,[&](double t) {
            VariableBindings b;
            (void)b.set("t",t);
            return ParametricPoint{px.evaluate(0,0,AngleMode::Radians,b),py.evaluate(0,0,AngleMode::Radians,b)};
        },0,2*pi);
        return Stats{s.evaluations,s.vertices.size()};
    });
    report("implicit_circle",repetitions,[&] {
        auto s=sampleImplicitContour(view,[](double x,double y){return x*x+y*y-9;},10,250000);
        return Stats{s.evaluations,s.vertices.size()};
    });
    report("inequality_boundary_and_spans",repetitions,[&] {
        auto s=sampleExplicitCurve(view,[](double x){return x*x-1;});
        std::size_t shaded=0;
        for(int x=0;x<view.width();x+=2) {
            const double world=view.screenToWorldX(x);
            const auto span=inequalityVerticalSpan(world*world-1,
                view.screenToWorldY(view.height()),view.screenToWorldY(0),view.height(),true);
            if(span)++shaded;
        }
        return Stats{s.evaluations+static_cast<std::size_t>((view.width()+1)/2),s.vertices.size()+shaded};
    });
    const auto cached=sampleParametricCurve(view,[](double t){return ParametricPoint{3*std::cos(t),3*std::sin(t)};},0,2*pi);
    report("trace_completed_geometry",repetitions,[&] {
        std::size_t hits=0;
        for(int i=0;i<256;++i){
            const double t=2*pi*i/256;
            const auto hit=nearestGraphTrace(cached.vertices,
                 view.worldToScreenX(3*std::cos(t)),view.worldToScreenY(3*std::sin(t)));
            hits+=static_cast<std::size_t>(hit.has_value());
        }
        return Stats{0,hits};
    });
    // Guard against accidental dead-code elimination in future revisions.
    if(observed==0) {std::cerr<<"No benchmark work observed\n";return 1;}
    return 0;
}
