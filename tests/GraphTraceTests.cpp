#include "GraphTrace.hpp"
#include "ParametricSampler.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

using namespace rationalstudio;
static int checks=0, failures=0;
void require(bool ok, const char* msg) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << msg << '\n'; }
}
bool near(double a, double b, double epsilon=1e-8) {return std::abs(a-b)<epsilon;}
int main() {
    const std::vector<CurveVertex> line{{0,0,true},{100,0,false}};
    auto h=nearestGraphTrace(line,25,4);
    require(h.has_value(), "find line");
    require(h && near(h->screenX,25),"project X");
    require(h && near(h->screenY,0),"project Y");
    require(h && near(h->distancePixels,4),"pixel distance");
    require(h && !h->hasParameter,"Cartesian has no parameter");
    require(!nearestGraphTrace(line,25,25),"reject distant point");
    require(!nearestGraphTrace(line,25,4,2),"enforce radius");
    require(!nearestGraphTrace(line,25,4,20,0),"zero vertex limit");
    const auto nan=std::numeric_limits<double>::quiet_NaN();
    require(!nearestGraphTrace(line,nan,0),"invalid cursor");
    require(!nearestGraphTrace(line,25,0,nan),"invalid radius");
    require(!nearestGraphTrace({},25,0),"empty curve");
    const std::vector<CurveVertex> split{{0,0,true},{10,0,false},{100,100,true},{110,100,false}};
    require(!nearestGraphTrace(split,55,50,15),"do not bridge moveTo discontinuity");
    h=nearestGraphTrace(split,101,103);
    require(h && h->distancePixels<4,"trace second branch");
    const std::vector<CurveVertex> p{{0,0,true,2},{100,0,false,4}};
    h=nearestGraphTrace(p,25,0);
    require(h && h->hasParameter,"parameter recovered");
    require(h && near(h->parameter,2.5),"linear parameter interpolated");
    const std::vector<CurveVertex> partial{{0,0,true,nan},{100,0,false,4}};
    h=nearestGraphTrace(partial,25,0);
    require(h && !h->hasParameter,"reject missing parameter on segment");
    require(!nearestGraphTrace(split,105,100,10,2),"bounded source scan");
    const std::vector<CurveVertex> singleton{{5,4,true,3}};
    h=nearestGraphTrace(singleton,5,4);
    require(h && h->hasParameter && near(h->parameter,3),"singleton parametric point");
    const std::vector<CurveVertex> bad{{nan,0,true},{10,0,false}};
    require(!nearestGraphTrace(bad,5,0,2),"reject nonfinite segment");
    GraphViewport view;
    require(view.setDimensions(500,400),"viewport initialized");
    const auto sampled=sampleParametricCurve(view,
        [](double t) {return ParametricPoint{std::cos(t),std::sin(t)};},0,6.283185307179586);
    require(!sampled.truncated,"reference circle complete");
    require(sampled.vertices.size()>30,"reference circle sampled");
    bool parameters=true;
    for (auto const& v:sampled.vertices)
        parameters &= std::isfinite(v.parameter) && v.parameter>=0 && v.parameter<=6.283185307179586;
    require(parameters,"parameter provenance retained for sampled vertices");
    h=nearestGraphTrace(sampled.vertices,view.worldToScreenX(0),view.worldToScreenY(1));
    require(h && h->hasParameter,"nearest circle sample includes parameter");
    require(h && std::abs(h->parameter - 1.57079632679)<0.1,"circle upper parameter matches t");
    std::cout<<(failures?"FAIL: ":"PASS: ")<<checks<<" GraphTrace assertions\n";
    return failures?1:0;
}
