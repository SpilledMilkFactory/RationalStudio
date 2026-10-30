#include "GraphFeatureDetection.hpp"

#include "IntersectionDetection.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <utility>

namespace rationalstudio {
namespace {
struct Sample { double x, y; };

class Inspector {
public:
    Inspector(const GraphFeatureFunction& function, const GraphFeatureOptions& options)
        : function_(function), options_(options) {}

    GraphFeatureResult run(double minX, double maxX) {
        if (!function_ || !std::isfinite(minX) || !std::isfinite(maxX) || !(minX < maxX) ||
            !std::isfinite(maxX-minX) || options_.subdivisions < 8 ||
            options_.subdivisions > 65536 || options_.maxEvaluations < 2 ||
            options_.maxEvaluations > 10000000 || options_.maxResults == 0 ||
            options_.maxResults > 10000 || !positive(options_.xTolerance) ||
            !positive(options_.residualTolerance) || !positive(options_.derivativeTolerance))
            return result_;
        step_ = (maxX-minX)/static_cast<double>(options_.subdivisions);
        if (!positive(step_)) return result_;
        result_.status = GraphFeatureStatus::Complete;

        // Pre-scan the finite domain once. Consecutive exact zeros imply a
        // sampled zero *interval*, not a list of isolated roots. Keep these
        // samples for extrema/stationarity analysis as well.
        std::vector<Sample> samples;
        samples.reserve(options_.subdivisions+1);
        for (std::size_t i=0; i<=options_.subdivisions; ++i) {
            Sample sample{i==options_.subdivisions ? maxX :
                          std::fma(static_cast<double>(i),step_,minX),0};
            if (!probe(sample.x,sample.y)) return sorted();
            if (!samples.empty() && finite(sample.y) && sample.y==0 &&
                finite(samples.back().y) && samples.back().y==0) {
                result_.status=GraphFeatureStatus::CoincidentWithAxis;
                return sorted();
            }
            samples.push_back(sample);
        }

        // Reuse Issue #40's tested sign-bracketing and tangency search; the
        // total function callback count includes the pre-scan above.
        IntersectionOptions roots;
        roots.subdivisions = options_.subdivisions;
        roots.maxEvaluations = std::min(std::size_t(10000000), options_.maxEvaluations * 2);
        roots.maxResults = options_.maxResults;
        roots.xTolerance = options_.xTolerance;
        roots.residualTolerance = options_.residualTolerance;
        std::size_t calls = result_.evaluations;
        const auto wrapped = [this, &calls](double x) -> double {
            // IntersectionDetection counts two callbacks, including its zero
            // callback. We count only actual evaluations of user f(x).
            if (calls >= options_.maxEvaluations)
                return std::numeric_limits<double>::quiet_NaN();
            ++calls;
            return invoke(x);
        };
        const auto zero = [](double) { return 0.; };
        const auto found = findCartesianIntersections(wrapped, zero, minX, maxX, roots);
        result_.evaluations = calls;
        result_.rejectedCandidates += found.rejectedCandidates;
        if (found.status == IntersectionStatus::Coincident) {
            result_.status = GraphFeatureStatus::CoincidentWithAxis;
            return result_;
        }
        if (found.status == IntersectionStatus::InvalidInput) {
            result_.status = GraphFeatureStatus::InvalidInput;
            return result_;
        }
        for (const auto& root : found.points)
            add({GraphFeatureKind::Root, root.x, root.y, root.residual, root.xBracketWidth});
        if (found.status == IntersectionStatus::EvaluationLimit ||
            result_.evaluations >= options_.maxEvaluations) {
            result_.status = GraphFeatureStatus::EvaluationLimit;
            return sorted();
        }
        if (found.status == IntersectionStatus::ResultLimit || stopped()) {
            result_.status = GraphFeatureStatus::ResultLimit;
            return sorted();
        }
        // A local sign change of consecutive secants brackets a candidate
        // extremum. Flat and undefined intervals are not classified.
        for (std::size_t i=1; i+1<samples.size() && !stopped(); ++i) {
            const auto& a=samples[i-1];
            const auto& m=samples[i];
            const auto& b=samples[i+1];
            if (!finite(a.y) || !finite(m.y) || !finite(b.y)) continue;
            if (m.y<a.y && m.y<b.y)
                extremum(a.x,b.x,GraphFeatureKind::LocalMinimum);
            else if (m.y>a.y && m.y>b.y)
                extremum(a.x,b.x,GraphFeatureKind::LocalMaximum);
        }
        // A sampled stationary inflection has a near-zero local minimum of
        // |df/dx| but no slope sign reversal. It is necessarily heuristic.
        for (std::size_t i=2; i+2<samples.size() && !stopped(); ++i) {
            const auto& a=samples[i-2]; const auto& b=samples[i-1];
            const auto& c=samples[i]; const auto& d=samples[i+1];
            const auto& e=samples[i+2];
            if (!finite(a.y)||!finite(b.y)||!finite(c.y)||!finite(d.y)||!finite(e.y)) continue;
            const double before=(c.y-a.y)/(c.x-a.x);
            const double mid=(d.y-b.y)/(d.x-b.x);
            const double after=(e.y-c.y)/(e.x-c.x);
            if (!finite(before)||!finite(mid)||!finite(after) ||
                before==0 || after==0 || std::signbit(before)!=std::signbit(after) ||
                !(std::abs(mid)<std::abs(before)) ||
                !(std::abs(mid)<std::abs(after))) continue;
            stationary(b.x,d.x);
        }
        return sorted();
    }

private:
    const GraphFeatureFunction& function_;
    const GraphFeatureOptions& options_;
    GraphFeatureResult result_;
    double step_ = 0;

    static bool finite(double n) { return std::isfinite(n); }
    static bool positive(double n) { return finite(n) && n>0; }
    bool stopped() const {
        return result_.status == GraphFeatureStatus::EvaluationLimit ||
               result_.status == GraphFeatureStatus::ResultLimit;
    }
    double invoke(double x) const {
        try { return function_(x); } catch (...) {
            return std::numeric_limits<double>::quiet_NaN();
        }
    }
    bool probe(double x, double& y) {
        if (result_.evaluations >= options_.maxEvaluations) {
            result_.status = GraphFeatureStatus::EvaluationLimit;
            return false;
        }
        ++result_.evaluations;
        y=invoke(x);
        return true;
    }
    void add(GraphFeaturePoint p) {
        if (!finite(p.x) || !finite(p.y) || !finite(p.residual)) {
            ++result_.rejectedCandidates;
            return;
        }
        for (auto& old:result_.points) {
            if (old.kind==p.kind && std::abs(old.x-p.x)<=
                std::max(options_.xTolerance*8,step_*0.01)) {
                if ((p.kind==GraphFeatureKind::LocalMinimum || p.kind==GraphFeatureKind::LocalMaximum)
                    ? p.residual>old.residual : p.residual<old.residual) old=p;
                return;
            }
        }
        if (result_.points.size()>=options_.maxResults) {
            result_.status=GraphFeatureStatus::ResultLimit;
            return;
        }
        result_.points.push_back(p);
    }
    GraphFeatureResult sorted() {
        std::sort(result_.points.begin(),result_.points.end(),
                  [](const auto& a,const auto& b) {
                      if (a.x!=b.x) return a.x<b.x;
                      return static_cast<int>(a.kind)<static_cast<int>(b.kind);
                  });
        return result_;
    }
    void extremum(double left, double right, GraphFeatureKind kind) {
        const double sign=kind==GraphFeatureKind::LocalMinimum ? 1. : -1.;
        const double initialLeft=left, initialRight=right;
        constexpr double k=0.3819660112501051518;
        Sample a{left+k*(right-left),0}, b{right-k*(right-left),0};
        if (!probe(a.x,a.y) || !probe(b.x,b.y)) return;
        if (!finite(a.y)||!finite(b.y)) { ++result_.rejectedCandidates; return; }
        Sample best=sign*a.y<sign*b.y?a:b;
        for (int iter=0;iter<70 && (right-left)>options_.xTolerance && !stopped();++iter) {
            if (sign*a.y<sign*b.y) {
                right=b.x; b=a; a.x=left+k*(right-left);
                if (!probe(a.x,a.y)) return;
                if (!finite(a.y)) { ++result_.rejectedCandidates; return; }
                if (sign*a.y<sign*best.y) best=a;
            } else {
                left=a.x; a=b; b.x=right-k*(right-left);
                if (!probe(b.x,b.y)) return;
                if (!finite(b.y)) { ++result_.rejectedCandidates; return; }
                if (sign*b.y<sign*best.y) best=b;
            }
        }
        // Verify strict local ordering at two flanking points, with a
        // meaningful finite separation (not numerical noise at the minimizer).
        const double delta=std::min(std::max(step_*0.25,options_.xTolerance*16),
            0.49*std::min(best.x-initialLeft,initialRight-best.x));
        if (!positive(delta)) { ++result_.rejectedCandidates; return; }
        double yl=0, yr=0;
        if (!probe(best.x-delta,yl) || !probe(best.x+delta,yr)) return;
        const double slack=16*std::numeric_limits<double>::epsilon() *
                          std::max({std::abs(best.y),std::abs(yl),std::abs(yr)});
        if (!finite(yl)||!finite(yr) ||
            !(sign*best.y+slack < sign*yl && sign*best.y+slack < sign*yr)) {
            ++result_.rejectedCandidates;
            return;
        }
        // A pole may mimic a minimum/maximum when adjacent grid samples
        // lie on opposite sides of a discontinuity. Reject a wildly spiking
        // refined value relative to its nearby finite reference probes.
        if (std::abs(best.y)>1000.*std::max({1.,std::abs(yl),std::abs(yr)})) {
            ++result_.rejectedCandidates;
            return;
        }
        const double margin=std::min(sign*(yl-best.y),sign*(yr-best.y));
        add({kind,best.x,best.y,margin,right-left});
    }
    bool derivative(double x, double h, double& y) {
        double before=0, after=0;
        if (!probe(x-h,before) || !probe(x+h,after)) return false;
        y=(after-before)/(2*h);
        return true;
    }
    void stationary(double left,double right) {
        constexpr double k=0.3819660112501051518;
        const double h=std::max(options_.xTolerance*8,step_*0.004);
        const double span=right-left;
        double x1=left+k*span, x2=right-k*span;
        double d1=0,d2=0;
        if (!derivative(x1,h,d1)||!derivative(x2,h,d2)) return;
        if (!finite(d1)||!finite(d2)) return;
        double bestX=std::abs(d1)<std::abs(d2)?x1:x2;
        double bestD=std::min(std::abs(d1),std::abs(d2));
        for(int i=0;i<60 && right-left>options_.xTolerance && !stopped();++i) {
            if(std::abs(d1)<std::abs(d2)) {
                right=x2; x2=x1; d2=d1; x1=left+k*(right-left);
                if(!derivative(x1,h,d1)) return;
                if(!finite(d1)) return;
                if(std::abs(d1)<bestD){bestD=std::abs(d1);bestX=x1;}
            } else {
                left=x1; x1=x2; d1=d2; x2=right-k*(right-left);
                if(!derivative(x2,h,d2)) return;
                if(!finite(d2)) return;
                if(std::abs(d2)<bestD){bestD=std::abs(d2);bestX=x2;}
            }
        }
        if(bestD>options_.derivativeTolerance) return;
        // Same-direction slopes well outside the derivative stencil are
        // needed to distinguish a flat turning point from a stationary inflection.
        double dl=0,dr=0;
        const double offset=step_*0.45;
        if(!derivative(bestX-offset,h,dl)||!derivative(bestX+offset,h,dr)) return;
        if(!finite(dl)||!finite(dr) || dl==0 || dr==0 ||
           std::signbit(dl)!=std::signbit(dr) ||
           !(bestD<0.25*std::min(std::abs(dl),std::abs(dr)))) {
            ++result_.rejectedCandidates; return;
        }
        double y=0;
        if(!probe(bestX,y)) return;
        add({GraphFeatureKind::StationaryInflection,bestX,y,bestD,right-left});
    }
};
} // namespace

GraphFeatureResult inspectCartesianFeatures(const GraphFeatureFunction& function,
       double minimum,double maximum,const GraphFeatureOptions& options) {
    return Inspector(function,options).run(minimum,maximum);
}
GraphFeatureResult inspectCartesianFeatures(const Expression& expression,
       double minimum,double maximum,AngleMode angle,const VariableBindings& bindings,
       double ans,const GraphFeatureOptions& options) {
    if(!expression.valid()) return {};
    return inspectCartesianFeatures(
        [&](double x){return expression.evaluate(x,ans,angle,bindings);},
        minimum,maximum,options);
}
} // namespace rationalstudio
