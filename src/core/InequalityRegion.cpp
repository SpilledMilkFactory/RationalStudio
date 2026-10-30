#include "InequalityRegion.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string_view>

namespace rationalstudio {
namespace {
std::string trim(std::string_view input) {
    while (!input.empty() && std::isspace(static_cast<unsigned char>(input.front())))
        input.remove_prefix(1);
    while (!input.empty() && std::isspace(static_cast<unsigned char>(input.back())))
        input.remove_suffix(1);
    return std::string(input);
}
bool isY(const std::string& text) {
    return text.size() == 1 && (text[0] == 'y' || text[0] == 'Y');
}
} // namespace

InequalitySource parseInequality(const std::string& raw) {
    InequalitySource result;
    const std::string text = trim(raw);
    const auto position = text.find_first_of("<>");
    // Also accept U+2264/U+2265 in the user-facing editor.
    const auto lessUnicode = text.find("\xE2\x89\xA4");
    const auto greaterUnicode = text.find("\xE2\x89\xA5");
    std::size_t pos = position;
    bool unicode = false;
    if (lessUnicode < pos) { pos = lessUnicode; unicode = true; }
    if (greaterUnicode < pos) { pos = greaterUnicode; unicode = true; }
    if (pos == std::string::npos) {
        result.error = "Expected y < f(x), y <= f(x), y > f(x), or y >= f(x).";
        return result;
    }
    const bool less = unicode ? pos == lessUnicode : text[pos] == '<';
    const bool inclusive = unicode || (pos + 1 < text.size() && text[pos + 1] == '=');
    const auto width = unicode ? 3u : (inclusive ? 2u : 1u);
    const std::string lhs = trim(std::string_view(text).substr(0, pos));
    const std::string rhs = trim(std::string_view(text).substr(pos + width));
    if (lhs.empty() || rhs.empty() || lhs == rhs ||
        (isY(lhs) == isY(rhs)) ||
        lhs.find_first_of("<>=") != std::string::npos ||
        rhs.find_first_of("<>=") != std::string::npos ||
        lhs.find("\xE2\x89\xA4") != std::string::npos ||
        lhs.find("\xE2\x89\xA5") != std::string::npos ||
        rhs.find("\xE2\x89\xA4") != std::string::npos ||
        rhs.find("\xE2\x89\xA5") != std::string::npos) {
        result.error = "One comparison with Cartesian y and a scalar boundary is required.";
        return result;
    }
    result.expression = isY(lhs) ? rhs : lhs;
    result.shadeAbove = isY(lhs) ? !less : less;
    result.inclusive = inclusive;
    return result;
}

std::optional<InequalitySpan> inequalityVerticalSpan(
    double boundaryY, double minY, double maxY, double pixelHeight,
    bool shadeAbove) noexcept {
    if (!std::isfinite(boundaryY) || !std::isfinite(minY) ||
        !std::isfinite(maxY) || !std::isfinite(pixelHeight) ||
        !(maxY > minY) || !(pixelHeight > 0)) return std::nullopt;
    if (shadeAbove && boundaryY >= maxY) return std::nullopt;
    if (!shadeAbove && boundaryY <= minY) return std::nullopt;
    // Clamp in WORLD SPACE before division. This prevents arithmetic overflow
    // for boundaries far beyond the view and makes fully-covered columns exact.
    double fraction = 0.;
    if (boundaryY <= minY) fraction = 1.;
    else if (boundaryY >= maxY) fraction = 0.;
    else {
        const double scale = std::max({std::abs(maxY), std::abs(minY),
                                       std::abs(boundaryY), 1.});
        fraction = ((maxY / scale) - (boundaryY / scale)) /
                   ((maxY / scale) - (minY / scale));
        if (!std::isfinite(fraction)) return std::nullopt;
        fraction = std::clamp(fraction, 0., 1.);
    }
    const double cut = fraction * pixelHeight;
    InequalitySpan result = shadeAbove ? InequalitySpan{0., cut}
                                       : InequalitySpan{cut, pixelHeight};
    if (!(result.bottom > result.top)) return std::nullopt;
    return result;
}

} // namespace rationalstudio
