#pragma once

#include <optional>
#include <string>

namespace rationalstudio {

// Presentation syntax restricted to y < f(x), y <= f(x), y > f(x), y >= f(x)
// and the equivalent reverse forms. The existing Expression parser validates
// the scalar boundary expression; this utility never changes its grammar.
struct InequalitySource {
    std::string expression;
    bool shadeAbove = false;
    bool inclusive = false;
    std::string error;
    [[nodiscard]] bool valid() const noexcept { return error.empty(); }
};

[[nodiscard]] InequalitySource parseInequality(const std::string& input);

struct InequalitySpan {
    double top = 0;
    double bottom = 0;
};

// Pixel-space clipped vertical fill span at one finite x sample. Returns
// nullopt for invalid parameters or a region outside the visible y range.
// minY/maxY are viewport world coordinates at the BOTTOM/TOP, respectively.
[[nodiscard]] std::optional<InequalitySpan> inequalityVerticalSpan(
    double boundaryY, double minY, double maxY, double pixelHeight,
    bool shadeAbove) noexcept;

} // namespace rationalstudio
