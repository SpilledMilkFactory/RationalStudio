#include "VariableSliderScale.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace rationalstudio {

std::optional<VariableSliderScale> VariableSliderScale::create(
    double minimum, double maximum, double step, std::string* error) noexcept {
    auto fail = [&](const char* reason) -> std::optional<VariableSliderScale> {
        if (error) *error = reason;
        return std::nullopt;
    };

    if (!std::isfinite(minimum) || !std::isfinite(maximum) ||
        !std::isfinite(step)) {
        return fail("Minimum, maximum, and step must be finite");
    }
    if (!(minimum < maximum)) {
        return fail("Minimum must be less than maximum");
    }
    if (!(step > 0.0)) {
        return fail("Step must be strictly positive");
    }

    long double count =
        (static_cast<long double>(maximum) - minimum) / step;
    // Range and step originate as doubles. Normalize nearly integral
    // quotients to avoid an artificial duplicate maximum tick (e.g. 60+eps).
    const long double rounded = std::round(count);
    const long double tolerance = 64 * std::numeric_limits<double>::epsilon() *
        std::max(1.0L, std::fabs(count));
    if (std::isfinite(count) && std::fabs(count - rounded) <= tolerance)
        count = rounded;
    if (!std::isfinite(count) || count > maxTicks) {
        return fail("Range requires more than 100000 slider ticks");
    }
    if (!(count > 0)) {
        return fail("Slider range cannot be represented");
    }
    // ceil includes maximum as a final, possibly shorter interval.
    const int ticks = std::max(1, static_cast<int>(std::ceil(count)));
    VariableSliderScale result;
    result.minimum_ = minimum;
    result.maximum_ = maximum;
    result.step_ = step;
    result.ticks_ = ticks;
    // Reject scales whose prescribed adjacent ticks collapse to the same
    // representable double near either endpoint.
    if (ticks > 1 && (result.valueAt(1) <= minimum ||
                      result.valueAt(ticks - 1) >= maximum)) {
        return fail("Step is too small for the selected endpoint precision");
    }
    if (error) error->clear();
    return result;
}

double VariableSliderScale::valueAt(int tick) const noexcept {
    if (tick <= 0) return minimum_;
    if (tick >= ticks_) return maximum_;
    const long double v = static_cast<long double>(minimum_) +
        static_cast<long double>(step_) * tick;
    return std::clamp(static_cast<double>(v), minimum_, maximum_);
}

int VariableSliderScale::nearestTick(double value) const noexcept {
    if (std::isnan(value) || value <= minimum_) return 0;
    if (value >= maximum_) return ticks_;
    // Use a long-double intermediate to avoid double overflow when subtracting
    // large finite endpoints. Compare neighbors explicitly, including the tail.
    const long double raw =
        (static_cast<long double>(value) - minimum_) / step_;
    const int left = std::clamp(static_cast<int>(std::floor(raw)), 0, ticks_);
    const int right = std::min(left + 1, ticks_);
    const long double ld = std::fabs(
        static_cast<long double>(value) - valueAt(left));
    const long double rd = std::fabs(
        static_cast<long double>(value) - valueAt(right));
    // At an exact midpoint, prefer the higher numerical value.
    return rd <= ld ? right : left;
}

} // namespace rationalstudio
