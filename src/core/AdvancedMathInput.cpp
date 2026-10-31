#include "AdvancedMathInput.hpp"

#include <charconv>
#include <cmath>
#include <string_view>

namespace rationalstudio {
namespace {
std::string_view trimmed(std::string_view text) {
    auto space = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!text.empty() && space(text.front())) text.remove_prefix(1);
    while (!text.empty() && space(text.back())) text.remove_suffix(1);
    return text;
}
}

ParsedNumberList parseAdvancedNumbers(const std::string& text, std::size_t limit) {
    ParsedNumberList result;
    if (limit == 0 || limit > 1'000'000) {
        result.error = "invalid element limit";
        return result;
    }
    std::string_view remaining(text);
    while (true) {
        const std::size_t comma = remaining.find(',');
        const auto token = trimmed(remaining.substr(0, comma));
        if (token.empty()) { result.error = "empty numeric field"; break; }
        if (result.values.size() >= limit) { result.error = "too many values"; break; }
        double number = 0.0;
        const auto [end, ec] = std::from_chars(token.data(), token.data() + token.size(), number);
        if (ec != std::errc{} || end != token.data() + token.size() || !std::isfinite(number)) {
            result.error = "invalid or non-finite number";
            break;
        }
        result.values.push_back(number);
        if (comma == std::string_view::npos) break;
        remaining.remove_prefix(comma + 1);
    }
    if (!result.error.empty()) result.values.clear();
    return result;
}

ParsedMatrix parseAdvancedMatrix(const std::string& text) {
    ParsedMatrix result;
    std::string_view remaining(text);
    std::size_t columns = 0;
    std::vector<double> entries;
    std::size_t rows = 0;
    while (true) {
        const auto delim = remaining.find(';');
        const auto row = trimmed(remaining.substr(0, delim));
        if (row.empty()) { result.error = "empty matrix row"; return result; }
        if (rows >= 128) { result.error = "matrix exceeds 128 rows"; return result; }
        const auto parsed = parseAdvancedNumbers(std::string(row), 128);
        if (!parsed.valid()) { result.error = parsed.error; return result; }
        if (columns == 0) columns = parsed.values.size();
        if (parsed.values.size() != columns) {
            result.error = "matrix rows have different column counts";
            return result;
        }
        entries.insert(entries.end(), parsed.values.begin(), parsed.values.end());
        ++rows;
        if (delim == std::string_view::npos) break;
        remaining.remove_prefix(delim + 1);
    }
    auto matrix = makeMatrix(rows, columns, entries);
    if (!matrix.successful()) { result.error = matrixStatusName(matrix.status); return result; }
    result.value = std::move(matrix.value);
    return result;
}

const char* advancedValueType(const AdvancedMathValue& value) noexcept {
    switch (value.index()) {
    case 0: return "scalar";
    case 1: return "complex";
    case 2: return "matrix";
    case 3: return "vector";
    case 4: return "statistics";
    case 5: return "regression";
    default: return "unknown";
    }
}
} // namespace rationalstudio
