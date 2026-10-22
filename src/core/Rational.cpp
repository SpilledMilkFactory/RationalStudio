#include "Rational.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace rationalstudio {

    namespace {

        using Integer = Rational::Integer;

        std::string trim(const std::string& s) {

            const auto start = s.find_first_not_of(" \t\r\n");

            if (start == std::string::npos) {
                return {};
            }

            const auto end = s.find_last_not_of(" \t\r\n");

            return s.substr(start, end - start + 1);
        }

        Integer parseInteger(const std::string& source) {

            const std::string input = trim(source);

            if (input.empty()) {
                throw std::invalid_argument("Missing integer");
            }

            std::size_t i = 0;
            bool negative = false;

            if (input[i] == '+' || input[i] == '-') {

                negative = input[i] == '-';
                ++i;
            }

            if (i == input.size()) {
                throw std::invalid_argument("Missing digits");
            }

            Integer n = 0;

            for (; i < input.size(); ++i) {

                const unsigned char c =
                static_cast<unsigned char>(input[i]);

                if (!std::isdigit(c)) {
                    throw std::invalid_argument(
                        "Expected a decimal integer"
                    );
                }

                n = n * 10 + (c - '0');
            }

            return negative ? -n : n;
        }

        Integer gcd(Integer a, Integer b) {

            if (a < 0) {
                a = -a;
            }

            if (b < 0) {
                b = -b;
            }

            while (b != 0) {

                Integer remainder = a % b;

                a = std::move(b);
                b = std::move(remainder);
            }

            return a;
        }

    } // namespace

    Rational::Rational()
    : numerator_(0),
    denominator_(1) {
    }

    Rational::Rational(
        Integer numerator,
        Integer denominator
    )
    : numerator_(std::move(numerator)),
    denominator_(std::move(denominator)) {

        normalize();
    }

    void Rational::normalize() {

        if (denominator_ == 0) {

            throw std::domain_error(
                "A fraction cannot have a zero denominator"
            );
        }

        if (denominator_ < 0) {

            numerator_ = -numerator_;
            denominator_ = -denominator_;
        }

        if (numerator_ == 0) {

            denominator_ = 1;
            return;
        }

        const Integer divisor = gcd(
            numerator_,
            denominator_
        );

        numerator_ /= divisor;
        denominator_ /= divisor;
    }

    Rational Rational::parse(const std::string& input) {

        const std::string s = trim(input);

        const auto slash = s.find('/');

        if (slash == std::string::npos) {

            return Rational(parseInteger(s));
        }

        if (s.find('/', slash + 1) != std::string::npos) {

            throw std::invalid_argument(
                "A fraction must have at most one slash"
            );
        }

        return Rational(
            parseInteger(s.substr(0, slash)),
                        parseInteger(s.substr(slash + 1))
        );
    }

    std::string Rational::toString() const {

        const std::string n =
        numerator_.convert_to<std::string>();

        return denominator_ == 1
        ? n
        : n + "/" + denominator_.convert_to<std::string>();
    }

    std::string Rational::toDecimal(unsigned places) const {

        if (places > 50) {

            throw std::invalid_argument(
                "Maximum decimal precision is 50 places"
            );
        }

        Integer positive =
        numerator_ < 0 ? -numerator_ : numerator_;

        Integer quotient = positive / denominator_;
        Integer remainder = positive % denominator_;

        std::string fractional;

        for (
            unsigned i = 0;
        i <= places && remainder != 0;
        ++i
        ) {

            remainder *= 10;

            const Integer digit =
            remainder / denominator_;

            remainder %= denominator_;

            fractional.push_back(
                static_cast<char>(
                    '0' + digit.convert_to<unsigned>()
                )
            );
        }

        // Round the last displayed decimal digit,
        // including carry into the integer part.

        if (fractional.size() > places) {

            const bool roundUp =
            fractional.back() >= '5';

            fractional.pop_back();

            if (roundUp) {

                bool carried = true;

                for (
                    std::size_t i = fractional.size();
                i > 0 && carried;
                --i
                ) {

                    if (fractional[i - 1] == '9') {

                        fractional[i - 1] = '0';

                    } else {

                        ++fractional[i - 1];
                        carried = false;
                    }
                }

                if (carried) {
                    ++quotient;
                }
            }
        }

        while (
            !fractional.empty() &&
            fractional.back() == '0'
        ) {

            fractional.pop_back();
        }

        std::string result =
        quotient.convert_to<std::string>();

        if (!fractional.empty()) {

            result += "." + fractional;
        }

        if (numerator_ < 0 && result != "0") {

            result.insert(result.begin(), '-');
        }

        return result;
    }

    Rational Rational::operator+(
        const Rational& rhs
    ) const {

        return Rational(
            numerator_ * rhs.denominator_ +
            rhs.numerator_ * denominator_,

            denominator_ * rhs.denominator_
        );
    }

    Rational Rational::operator-(
        const Rational& rhs
    ) const {

        return Rational(
            numerator_ * rhs.denominator_ -
            rhs.numerator_ * denominator_,

            denominator_ * rhs.denominator_
        );
    }

    Rational Rational::operator*(
        const Rational& rhs
    ) const {

        return Rational(
            numerator_ * rhs.numerator_,
            denominator_ * rhs.denominator_
        );
    }

    Rational Rational::operator/(
        const Rational& rhs
    ) const {

        if (rhs.numerator_ == 0) {

            throw std::domain_error(
                "Division by zero"
            );
        }

        return Rational(
            numerator_ * rhs.denominator_,
            denominator_ * rhs.numerator_
        );
    }

    bool Rational::operator==(
        const Rational& rhs
    ) const noexcept {

        return numerator_ == rhs.numerator_ &&
        denominator_ == rhs.denominator_;
    }

    bool Rational::operator<(
        const Rational& rhs
    ) const {

        return numerator_ * rhs.denominator_ <
        rhs.numerator_ * denominator_;
    }

} // namespace rationalstudio
