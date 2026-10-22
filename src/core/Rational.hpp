#pragma once

#include <boost/multiprecision/cpp_int.hpp>
#include <string>

namespace rationalstudio {

    class Rational {
    public:

        using Integer = boost::multiprecision::cpp_int;

        Rational();

        Rational(
            Integer numerator,
            Integer denominator = 1
        );

        static Rational parse(const std::string& input);

        const Integer& numerator() const noexcept {
            return numerator_;
        }

        const Integer& denominator() const noexcept {
            return denominator_;
        }

        std::string toString() const;

        std::string toDecimal(unsigned places = 15) const;

        Rational operator+(const Rational& rhs) const;

        Rational operator-(const Rational& rhs) const;

        Rational operator*(const Rational& rhs) const;

        Rational operator/(const Rational& rhs) const;

        bool operator==(const Rational& rhs) const noexcept;

        bool operator<(const Rational& rhs) const;

    private:

        Integer numerator_;
        Integer denominator_;

        void normalize();
    };

} // namespace rationalstudio
