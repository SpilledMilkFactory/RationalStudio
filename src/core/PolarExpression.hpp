#pragma once

#include "MathExpressionInput.hpp"

#include <cctype>
#include <string>
#include <string_view>

namespace rationalstudio {

// This is an input adapter, not a second expression grammar or renderer.
// Local polar angle aliases theta and UTF-8 θ are compiled as parameter t.
// The caller supplies t through a per-sample VariableBindings snapshot.
struct PolarCoordinateSources {
    std::string radius;
    std::string x;
    std::string y;
    [[nodiscard]] bool valid() const noexcept { return !radius.empty(); }
};

inline PolarCoordinateSources polarCoordinateSources(std::string_view input) {
    auto trim = [](std::string_view s) {
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.remove_prefix(1);
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.remove_suffix(1);
        return s;
    };
    input = trim(input);
    const auto equal = input.find('=');
    if (equal != std::string_view::npos) {
        const auto label = trim(input.substr(0, equal));
        if (label.size() != 1 || (label[0] != 'r' && label[0] != 'R')) return {};
        input = trim(input.substr(equal + 1));
    }
    if (input.empty()) return {};
    // Normalize ordinary mathematical glyphs first, then translate only
    // complete angle identifiers (never substrings of another identifier).
    const std::string source = MathExpressionInput::normalize(input);
    std::string radius;
    radius.reserve(source.size());
    auto identifier = [](unsigned char c) {
        return std::isalnum(c) || c == '_';
    };
    for (std::size_t i = 0; i < source.size();) {
        const auto u = static_cast<unsigned char>(source[i]);
        if (source.compare(i, 2, "\xCE\xB8") == 0 ||
            source.compare(i, 2, "\xCE\x98") == 0) { // θ / Θ
            radius += 't';
            i += 2;
            continue;
        }
        if (std::isalpha(u) || u == '_') {
            std::size_t j = i + 1;
            while (j < source.size() && identifier(static_cast<unsigned char>(source[j]))) ++j;
            const auto word = source.substr(i, j-i);
            std::string lower = word;
            for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            radius += (lower == "theta" ? "t" : word);
            i = j;
            continue;
        }
        radius.push_back(source[i++]);
    }
    if (radius.empty()) return {};
    // Multiplication is explicit: no dependence on parser implicit-product
    // heuristics. The same radius expression participates in both axes.
    return {radius, "(" + radius + ")*cos(t)", "(" + radius + ")*sin(t)"};
}

} // namespace rationalstudio
