// Milestone 3 / Issue #40: numerical intersections with finite domains,
// documented residuals, deterministic budgets, and independent references.
#include "IntersectionDetection.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace rationalstudio;
namespace {
int assertions = 0;
int failures = 0;
void check(bool ok, const std::string& what) {
    ++assertions;
    if (!ok) { ++failures; std::cerr << "FAIL: " << what << '\n'; }
}
void close(double actual, double expected, double tolerance, const std::string& what) {
    check(std::isfinite(actual) && std::abs(actual-expected) <= tolerance, what);
}
CartesianIntersectionFunction constant(double n) {
    return [n](double) { return n; };
}
IntersectionOptions standard() { return {}; }
}
int main() {
    const auto zero = constant(0);
    const auto line = CartesianIntersectionFunction([](double x) { return x; });
    const auto parabola = CartesianIntersectionFunction([](double x) { return x*x; });
    const auto xminus2 = CartesianIntersectionFunction([](double x) { return x-2; });
    const auto sine = CartesianIntersectionFunction([](double x) { return std::sin(x); });
    const auto invalid = [](double x) { return x == 0 ? std::numeric_limits<double>::quiet_NaN() : 1/x; };

    {
        const auto result=findCartesianIntersections(line, zero,-2,2);
        check(result.complete(), "line complete");
        check(result.points.size()==1, "line/axis one intersection");
        if(result.points.size()==1) {
            close(result.points[0].x,0,1e-7,"line intersection x");
            close(result.points[0].y,0,1e-7,"line intersection y");
            check(result.points[0].residual <= 1e-8,"reported residual meets limit");
        }
        check(result.evaluations >= 2*513,"both callbacks accounted");
        check(result.evaluations <= standard().maxEvaluations,"budget respected");
    }
    {
        const auto result=findCartesianIntersections(parabola,line,-1,2);
        check(result.complete(),"parabola-line complete");
        check(result.points.size()==2,"parabola/line two analytical intersections");
        if(result.points.size()==2) {
            close(result.points[0].x,0,1e-6,"parabola/line x0");
            close(result.points[1].x,1,1e-6,"parabola/line x1");
            close(result.points[1].y,1,1e-6,"parabola/line y1");
        }
    }
    {
        const auto result=findCartesianIntersections(parabola,constant(0.1369),-2,2);
        check(result.complete(),"positive level set complete");
        check(result.points.size()==2,"parabola positive level two roots");
        if(result.points.size()==2) {
            close(result.points[0].x,-0.37,1e-6,"negative level crossing");
            close(result.points[1].x, 0.37,1e-6,"positive level crossing");
        }
    }
    {
        const auto touching=CartesianIntersectionFunction([](double x){return (x-0.371)*(x-0.371);});
        const auto result=findCartesianIntersections(touching,zero,-1,1);
        check(result.complete(),"tangency search complete");
        check(result.points.size()==1,"unsampled tangent root found");
        if(!result.points.empty()) {
            close(result.points[0].x,0.371,2e-4,"tangent position");
            check(result.points[0].residual <=1e-8,"tangent actual residual qualified");
        }
    }
    {
        const auto result=findCartesianIntersections(sine,zero,-7,7);
        check(result.complete(),"sine complete");
        check(result.points.size()==5,"sine detects five isolated roots");
        if(result.points.size()==5) {
            for(int i=-2;i<=2;++i)
                close(result.points[static_cast<std::size_t>(i+2)].x,
                      i*3.14159265358979323846,1e-6,"sine root analytical reference");
        }
    }
    {
        const auto result=findCartesianIntersections(
            CartesianIntersectionFunction(invalid),zero,-1,1);
        check(result.complete(),"discontinuous reciprocal complete");
        check(result.points.empty(),"reciprocal pole not fabricated as zero");
    }
    {
        const auto result=findCartesianIntersections(
            CartesianIntersectionFunction([](double x){return x < 0 ? -1. : 1.;}),zero,-1,1);
        check(result.complete(),"finite step scan complete");
        check(result.points.empty(),"discontinuous jump not fabricated as zero");
    }
    {
        const auto result=findCartesianIntersections(parabola,constant(-1),-2,2);
        check(result.complete() && result.points.empty(),"no real intersections");
    }
    {
        const auto result=findCartesianIntersections(line,line,-2,2);
        check(result.status==IntersectionStatus::Coincident,"identical curves not 64 arbitrary roots");
        check(result.points.empty(),"coincident has no fabricated isolated roots");
    }
    {
        auto options=standard();options.subdivisions=512;options.maxEvaluations=20;
        const auto result=findCartesianIntersections(line,zero,-2,2,options);
        check(result.status==IntersectionStatus::EvaluationLimit,"strict callback cap reported");
        check(result.evaluations==20,"strict callback count maintained");
    }
    {
        auto options=standard();options.maxResults=2;
        const auto result=findCartesianIntersections(sine,zero,-7,7,options);
        check(result.status==IntersectionStatus::ResultLimit,"root cap reported");
        check(result.points.size()==2,"result cap enforced");
    }
    {
        auto options=standard();options.subdivisions=3;
        check(findCartesianIntersections(line,zero,-1,1,options).status==IntersectionStatus::InvalidInput,
              "too few subdivisions rejected");
        options=standard();options.xTolerance=-1;
        check(findCartesianIntersections(line,zero,-1,1,options).status==IntersectionStatus::InvalidInput,
              "negative X tolerance rejected");
        options=standard();options.residualTolerance=std::numeric_limits<double>::quiet_NaN();
        check(findCartesianIntersections(line,zero,-1,1,options).status==IntersectionStatus::InvalidInput,
              "NaN residual tolerance rejected");
        check(findCartesianIntersections(line,zero,2,-1).status==IntersectionStatus::InvalidInput,
              "reversed domain rejected");
        check(findCartesianIntersections(line,zero,1,1).status==IntersectionStatus::InvalidInput,
              "degenerate domain rejected");
        check(findCartesianIntersections(line,zero,-INFINITY,1).status==IntersectionStatus::InvalidInput,
              "infinite domain rejected");
        check(findCartesianIntersections({},zero,-1,1).status==IntersectionStatus::InvalidInput,
              "missing callback rejected");
    }
    {
        Expression f("x^2-1");
        Expression g("0");
        VariableBindings bindings;
        const auto result=findCartesianIntersections(f,g,-2,2,AngleMode::Radians,bindings);
        check(result.complete() && result.points.size()==2,"compiled Expression overload");
        if(result.points.size()==2) {
            close(result.points[0].x,-1,1e-6,"compiled x-1 root");
            close(result.points[1].x,1,1e-6,"compiled x+1 root");
        }
    }
    {
        VariableScope scope;
        check(scope.declare("a"),"declare parameter a");
        Expression f("a*x",scope),g("x+1",scope);
        VariableBindings binding;
        check(binding.set("a",2),"set a=2");
        auto first=findCartesianIntersections(f,g,-3,3,AngleMode::Radians,binding);
        check(first.complete() && first.points.size()==1,"bound variable intersection");
        if(!first.points.empty()) close(first.points[0].x,1,1e-6,"a=2 intersection");
        check(binding.set("a",3),"update a=3");
        auto second=findCartesianIntersections(f,g,-3,3,AngleMode::Radians,binding);
        check(second.complete() && second.points.size()==1,"no reparse after binding update");
        if(!second.points.empty()) close(second.points[0].x,0.5,1e-6,"a=3 intersection");
    }
    {
        // Probe reproducibility with independently generated line pairs.
        for(int i=1;i<=25;++i) {
            const double root=-1.0+0.08*i;
            const auto first=CartesianIntersectionFunction([=](double x){return 3*x-3*root;});
            const auto result=findCartesianIntersections(first,zero,-2,2);
            check(result.complete(),"sweep complete");
            if (result.points.size()!=1) { std::cerr << "sweep i=" << i << " roots=" << result.points.size(); for (auto& p: result.points) std::cerr << " (" << p.x << ", " << p.residual << ")"; std::cerr << "\n"; }
            check(result.points.size()==1,"sweep exactly one root");
            if(!result.points.empty()) {
                close(result.points[0].x,root,1e-6,"sweep analytical root");
                check(result.points[0].residual<=1e-8,"sweep residual certified");
            }
        }
    }
    std::cout<<(failures?"FAIL: ":"PASS: ")<<assertions<<" IntersectionDetection assertions\n";
    return failures?1:0;
}
