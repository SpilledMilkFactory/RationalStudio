#pragma once

#include <optional>
#include <string>

namespace rationalstudio {

// Qt-free mapping between bounded integer slider ticks and finite doubles.
// The final tick always reaches maximum, even when the range is not evenly
// divisible by step; the terminal interval may therefore be shorter.
class VariableSliderScale {
public:
    static constexpr int maxTicks = 100000;

    static std::optional<VariableSliderScale> create(
        double minimum, double maximum, double step,
        std::string* error = nullptr) noexcept;

    double minimum() const noexcept { return minimum_; }
    double maximum() const noexcept { return maximum_; }
    double step() const noexcept { return step_; }
    int ticks() const noexcept { return ticks_; }

    double valueAt(int tick) const noexcept;
    int nearestTick(double value) const noexcept;

private:
    double minimum_ = 0;
    double maximum_ = 1;
    double step_ = 1;
    int ticks_ = 1;
};

} // namespace rationalstudio
