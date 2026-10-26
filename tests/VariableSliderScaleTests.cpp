#include "VariableSliderScale.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

using namespace rationalstudio;

namespace {
int checks = 0;
int failures = 0;
void check(bool ok, const std::string& label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
void near(double a, double b, const std::string& label) {
    check(std::isfinite(a) && std::abs(a - b) < 1e-8, label);
}
}
int main() {
    std::string error;
    check(!VariableSliderScale::create(1, 1, .1, &error).has_value() && !error.empty(), "zero span");
    check(!VariableSliderScale::create(2, -2, .1), "reverse span");
    check(!VariableSliderScale::create(-1, 1, 0), "zero step");
    check(!VariableSliderScale::create(-1, 1, -.1), "negative step");
    check(!VariableSliderScale::create(-1, 1, std::numeric_limits<double>::quiet_NaN()), "nan step");
    check(!VariableSliderScale::create(-1, 1, std::numeric_limits<double>::infinity()), "inf step");
    check(!VariableSliderScale::create(-1, std::numeric_limits<double>::infinity(), 1), "inf max");
    check(!VariableSliderScale::create(0, 1, .000001), "cap tick count");
    check(!VariableSliderScale::create(1e9, 1e9 + 1e-5, 1e-9), "unrepresentable precision");

    const auto scale = VariableSliderScale::create(-2, 2, .25, &error);
    check(scale.has_value() && error.empty(), "valid scale");

    if (!scale) {
        std::cerr << "FAIL: Unable to construct baseline slider scale\n";
        return 1;
    }

    check(scale->ticks() == 16, "tick count");
    near(scale->valueAt(0), -2, "first endpoint");
    near(scale->valueAt(16), 2, "last endpoint");
    near(scale->valueAt(-100), -2, "low clamp");
    near(scale->valueAt(1000), 2, "high clamp");
    check(scale->nearestTick(-9) == 0 && scale->nearestTick(9) == 16, "nearest clamp");
    check(scale->nearestTick(std::numeric_limits<double>::quiet_NaN()) == 0, "NaN nearest");

    for (int i = 0; i <= 16; ++i) {
        near(scale->valueAt(i), -2 + .25 * i, "grid value");
        check(scale->nearestTick(scale->valueAt(i)) == i, "roundtrip");
    }
    const auto odd = VariableSliderScale::create(0, 1, .3);
    check(odd.has_value() && odd->ticks() == 4, "uneven step cap");
    near(odd->valueAt(3), .9, "penultimate");
    near(odd->valueAt(4), 1, "terminal tail");
    check(odd->nearestTick(.98) == 4, "tail nearest");
    check(odd->nearestTick(.91) == 3, "tail nearest early");
    const auto wide = VariableSliderScale::create(-1e9, 1e9, 2e7);
    check(wide.has_value() && wide->ticks() == 100, "wide finite range");
    // Issue #30: Midpoint snapping must prefer the greater value.
    const auto midpoint = VariableSliderScale::create(-5, 5, 2);

    check(midpoint.has_value(), "midpoint scale created");

    if (midpoint) {
        for (int tick = 0; tick < midpoint->ticks(); ++tick) {
            const double between =
            (midpoint->valueAt(tick) +
            midpoint->valueAt(tick + 1)) / 2.0;

            check(
                midpoint->nearestTick(between) == tick + 1,
                  "midpoint snaps to greater numerical value"
            );
        }
    }
    const auto one = VariableSliderScale::create(0, .3, 1);
    check(one.has_value() && one->ticks() == 1, "single tick");
    near(one->valueAt(1), .3, "single endpoint");
    // Stable deterministic sweep across positive and negative endpoints.
    for (int caseNo = 1; caseNo <= 80; ++caseNo) {
        const double minimum = -caseNo * 0.125;
        const double maximum = caseNo * 0.625;
        const double step = caseNo * .0125;
        auto s = VariableSliderScale::create(minimum, maximum, step);
        check(s.has_value(), "sweep creation");
        if (!s) continue;
        check(s->ticks() == 60, "sweep tick count");
        for (int i = 0; i <= s->ticks(); ++i) {
            const double v = s->valueAt(i);
            check(v >= minimum && v <= maximum, "sweep bounds");
            check(s->nearestTick(v) == i, "sweep roundtrip");
            if (i > 0) check(v > s->valueAt(i - 1), "sweep monotonicity");
        }
    }
    std::cout << (failures ? "FAIL: " : "PASS: ") << checks
              << " VariableSliderScale assertions\n";
    return failures ? 1 : 0;
}
