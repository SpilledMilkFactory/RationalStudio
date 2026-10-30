#include "NumericalDifferentiation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

namespace rationalstudio {
namespace {

bool positiveFinite(double value) { return std::isfinite(value) && value > 0; }

bool valid(const DifferentiationFunction& f, double x, const DifferentiationOptions& o) {
    return static_cast<bool>(f) && std::isfinite(x) &&
        positiveFinite(o.initialRelativeStep) &&
        positiveFinite(o.minimumRelativeStep) &&
        o.minimumRelativeStep < o.initialRelativeStep &&
        positiveFinite(o.absoluteTolerance) &&
        positiveFinite(o.relativeTolerance) &&
        positiveFinite(o.sideAgreementTolerance) &&
        o.maxRefinements >= 3 && o.maxRefinements <= 64 &&
        o.maxEvaluations >= 5 && o.maxEvaluations <= 1000000;
}

struct Probe {
    const DifferentiationFunction& callback;
    std::size_t limit;
    std::size_t evaluations = 0;
    bool exhausted = false;
    std::map<double, double> cache;

    double at(double x) {
        if (!std::isfinite(x)) return std::numeric_limits<double>::quiet_NaN();
        if (auto it=cache.find(x); it!=cache.end()) return it->second;
        if (evaluations >= limit) {
            exhausted = true;
            return std::numeric_limits<double>::quiet_NaN();
        }
        ++evaluations;
        // Callback exception handling is deliberate: this is an ordinary
        // numerical failure, not a reason to crash the caller.
        double y = std::numeric_limits<double>::quiet_NaN();
        try { y = callback(x); } catch (...) { /* undefined evaluation */ }
        cache.emplace(x,y);
        return y;
    }
};

struct Candidate {
    DerivativeMethod method = DerivativeMethod::None;
    double raw = 0;
    double left = std::numeric_limits<double>::quiet_NaN();
    double right = std::numeric_limits<double>::quiet_NaN();
    bool available = false;
};

bool agreement(double a, double b, double threshold) {
    return std::abs(a-b) <= threshold * std::max({1.0,std::abs(a),std::abs(b)});
}

} // namespace

DerivativeResult differentiate(const DifferentiationFunction& f, double x,
                               const DifferentiationOptions& o) {
    DerivativeResult result;
    if (!valid(f,x,o)) return result;
    Probe probe{f,o.maxEvaluations,0,false,{}};
    const double f0=probe.at(x);
    result.evaluations=probe.evaluations;
    if (!std::isfinite(f0)) { result.status=DerivativeStatus::UndefinedAtPoint; return result; }

    const double scale=std::max(1.0,std::abs(x));
    double h=o.initialRelativeStep*scale;
    if (!positiveFinite(h)) { result.status=DerivativeStatus::NoFiniteStencil; return result; }
    const double minStep=std::max(o.minimumRelativeStep*scale,
                                 16*std::numeric_limits<double>::epsilon()*scale);
    double previousRaw=0;
    double previousExtrap=0;
    DerivativeMethod previousMethod=DerivativeMethod::None;
    bool havePreviousRaw=false;
    bool havePreviousExtrap=false;
    double bestError=std::numeric_limits<double>::infinity();
    bool anyStencil=false, observedMismatch=false;

    for (std::size_t k=0;k<o.maxRefinements && h>=minStep;++k,h*=0.5) {
        result.refinements=k+1;
        // Require two representable, distinct abscissas. Otherwise floating-
        // point roundoff would silently turn the stencil into f(x)-f(x).
        const double xp=x+h, xm=x-h;
        if (!std::isfinite(xp) || !std::isfinite(xm) || xp==x || xm==x) break;
        const double plus=probe.at(xp), minus=probe.at(xm);
        Candidate c;
        if (std::isfinite(plus) && std::isfinite(minus)) {
            c.method=DerivativeMethod::CentralRichardson;
            c.raw=(plus-minus)/(2*h);
            c.left=(f0-minus)/h;
            c.right=(plus-f0)/h;
            c.available=true;
        } else if (o.allowOneSided && std::isfinite(plus) && !std::isfinite(minus)) {
            const double p2=probe.at(x+2*h);
            if (std::isfinite(p2)) {
                c.method=DerivativeMethod::ForwardRichardson;
                c.raw=(-3*f0+4*plus-p2)/(2*h);
                c.available=true;
            }
        } else if (o.allowOneSided && !std::isfinite(plus) && std::isfinite(minus)) {
            const double m2=probe.at(x-2*h);
            if (std::isfinite(m2)) {
                c.method=DerivativeMethod::BackwardRichardson;
                c.raw=(3*f0-4*minus+m2)/(2*h);
                c.available=true;
            }
        }
        if (probe.exhausted) break;
        if (!c.available || !std::isfinite(c.raw)) {
            havePreviousRaw=false;
            havePreviousExtrap=false;
            previousMethod=DerivativeMethod::None;
            continue;
        }
        anyStencil=true;
        const bool hasPair=havePreviousRaw && previousMethod==c.method;
        if (hasPair) {
            const double extrap=(4*c.raw-previousRaw)/3;
            if (std::isfinite(extrap)) {
                const double error=havePreviousExtrap
                    ? std::abs(extrap-previousExtrap)
                    : std::abs(extrap-c.raw);
                const double tolerance=o.absoluteTolerance+
                    o.relativeTolerance*std::max(1.0,std::abs(extrap));
                const bool leftRightOkay=c.method!=DerivativeMethod::CentralRichardson ||
                    agreement(c.left,c.right,o.sideAgreementTolerance);
                if (!leftRightOkay) observedMismatch=true;
                if (error<bestError) {
                    bestError=error;
                    result.value=extrap;
                    result.estimatedError=error;
                    result.method=c.method;
                    result.step=h;
                }
                if (havePreviousExtrap && error<=tolerance && leftRightOkay) {
                    result.status=DerivativeStatus::Converged;
                    result.value=extrap;
                    result.estimatedError=error;
                    result.method=c.method;
                    result.step=h;
                    result.evaluations=probe.evaluations;
                    return result;
                }
                previousExtrap=extrap;
                havePreviousExtrap=true;
            }
        } else havePreviousExtrap=false;
        previousRaw=c.raw;
        previousMethod=c.method;
        havePreviousRaw=true;
    }
    result.evaluations=probe.evaluations;
    if (probe.exhausted) result.status=DerivativeStatus::EvaluationLimit;
    else if (!anyStencil) result.status=DerivativeStatus::NoFiniteStencil;
    else if (observedMismatch) result.status=DerivativeStatus::NonDifferentiable;
    else result.status=DerivativeStatus::Unstable;
    // On failure, do not expose a plausible but unqualified number as a result.
    result.value=std::numeric_limits<double>::quiet_NaN();
    result.estimatedError=std::numeric_limits<double>::infinity();
    result.method=DerivativeMethod::None;
    result.step=0;
    return result;
}

DerivativeResult differentiate(const Expression& e, double x, AngleMode angle,
                               const VariableBindings& bindings, double ans,
                               const DifferentiationOptions& options) {
    if (!e.valid() || !std::isfinite(ans)) return {};
    return differentiate([&](double at){return e.evaluate(at,ans,angle,bindings);},x,options);
}

const char* derivativeStatusName(DerivativeStatus s) noexcept {
    switch(s) {
        case DerivativeStatus::Converged: return "converged";
        case DerivativeStatus::InvalidInput: return "invalid input";
        case DerivativeStatus::UndefinedAtPoint: return "undefined at point";
        case DerivativeStatus::EvaluationLimit: return "evaluation limit";
        case DerivativeStatus::NoFiniteStencil: return "no finite stencil";
        case DerivativeStatus::NonDifferentiable: return "nondifferentiable candidate";
        case DerivativeStatus::Unstable: return "unstable or unresolved";
    }
    return "unknown";
}

const char* derivativeMethodName(DerivativeMethod m) noexcept {
    switch(m) {
        case DerivativeMethod::None: return "none";
        case DerivativeMethod::CentralRichardson: return "central Richardson";
        case DerivativeMethod::ForwardRichardson: return "forward Richardson";
        case DerivativeMethod::BackwardRichardson: return "backward Richardson";
    }
    return "unknown";
}

} // namespace rationalstudio
