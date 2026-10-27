#pragma once

#include <string>
#include <string_view>
#include <cctype>

namespace rationalstudio {

    // Presentation-only translation. The compiled mathematical grammar is unchanged.
    // UTF-8 glyphs remain visible in the editor and are normalized only at its
    // parser boundary. This API never rewrites a user's text or cursor position.
    struct MathExpressionInput final {
        static std::string normalize(std::string_view input) {
            std::string result;
            result.reserve(input.size());
            for (std::size_t i = 0; i < input.size();) {
                auto match = [&](std::string_view token, std::string_view replacement) {
                    if (input.substr(i, token.size()) != token) return false;
                    result.append(replacement);
                    i += token.size();
                    return true;
                };
                if (match("\xCF\x80", "pi") ||        // π
                    match("\xCF\x84", "tau") ||       // τ
                    match("\xC3\x97", "*") ||        // ×
                    match("\xC3\xB7", "/") ||        // ÷
                    match("\xE2\x88\x92", "-") ||   // −
                    match("\xE2\x88\x99", "*") ||   // ∙
                    match("\xC2\xB7", "*") ||        // ·
                    match("\xC2\xB2", "^2") ||       // ²
                    match("\xC2\xB3", "^3") ||       // ³
                    match("\xE2\x88\x9A", "sqrt")) { // √, requires parentheses
                        continue;
                    }
                    result.push_back(input[i++]);
            }
            return result;
        }

        // Only a hint for an incomplete edit. The authoritative parser decides
        // whether a completed expression is valid; this is NOT syntax validation.
        static bool looksIncomplete(std::string_view normalized) {
            const std::size_t end =
            normalized.find_last_not_of(" \t\r\n");

            if (end == std::string_view::npos)
                return true;

            // Issue #32:
            // Distinguish incomplete mathematical expressions from
            // irrecoverably malformed operator sequences.
            //
            // For example:
            //   "2+"    -> incomplete
            //   "sin("  -> incomplete
            //   "2+*"   -> invalid
            //   "2**"   -> invalid
            //
            // The Expression parser remains authoritative for
            // mathematical syntax validation.

            for (std::size_t i = 0; i <= end; ++i) {
                const char c = normalized[i];

                // These operators cannot begin an operand.
                if (c != '*' &&
                    c != '/' &&
                    c != '^' &&
                    c != '%') {
                    continue;
                    }

                    // Locate the preceding non-whitespace character.
                    const std::size_t previous =
                    (i == 0)
                    ? std::string_view::npos
                    : normalized.find_last_not_of(
                        " \t\r\n", i - 1
                    );

                    // A binary operator is invalid when it appears
                    // without a preceding operand.
                    if (previous == std::string_view::npos ||
                        normalized[previous] == '(' ||
                        normalized[previous] == ',' ||
                        normalized[previous] == '+' ||
                        normalized[previous] == '-' ||
                        normalized[previous] == '*' ||
                        normalized[previous] == '/' ||
                        normalized[previous] == '^' ||
                        normalized[previous] == '%') {

                        return false;
                        }
            }

            const char last = normalized[end];

            // A trailing operator may represent an unfinished edit.
            if (last == '+' ||
                last == '-' ||
                last == '*' ||
                last == '/' ||
                last == '^' ||
                last == '%' ||
                last == '(' ||
                last == ',') {

                return true;
                }

                // Check for unclosed parentheses.
                int depth = 0;

                for (char c : normalized) {
                    if (c == '(')
                        ++depth;

                    if (c == ')')
                        --depth;
                }

                // Empty argument lists and incomplete arguments
                // are treated as unfinished input.
                const auto expression = normalized.substr(0, end + 1);

                if (expression.ends_with("()") ||
                    expression.ends_with(",)")) {

                    return true;
                    }

                    return depth > 0;
        }
    };

} // namespace rationalstudio
