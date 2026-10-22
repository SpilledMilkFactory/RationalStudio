// Terminal interface for exact rational arithmetic.
// Shares the RationalCore library with the Qt application.

#include "Rational.hpp"

#include <exception>
#include <iostream>
#include <string>

int main() {

    using rationalstudio::Rational;

    std::cout
    << "Welcome to Rational Studio's "
    << "original-style fraction calculator!\n"
    << "Enter two rational numbers separated by "
    << "a space (for example: 5/2 7/4):\n> ";

    std::string first;
    std::string second;

    if (!(std::cin >> first >> second)) {

        std::cerr << "Input required.\n";

        return 1;
    }

    std::cout
    << "Press + to add, - to subtract, "
    << "* to multiply, / to divide:\n> ";

    char operation = '\0';

    if (!(std::cin >> operation)) {

        std::cerr << "Operation required.\n";

        return 1;
    }

    try {

        const Rational a =
        Rational::parse(first);

        const Rational b =
        Rational::parse(second);

        Rational result;

        switch (operation) {

            case '+':
                result = a + b;
                break;

            case '-':
                result = a - b;
                break;

            case '*':
                result = a * b;
                break;

            case '/':
                result = a / b;
                break;

            default:

                std::cerr
                << "Unknown operation.\n";

                return 1;
        }

        std::cout
        << a.toString()
        << ' '
        << operation
        << ' '
        << b.toString()
        << " = "
        << result.toString()
        << '\n';

    } catch (const std::exception& error) {

        std::cerr
        << "Calculation error: "
        << error.what()
        << '\n';

        return 1;
    }

    return 0;
}
