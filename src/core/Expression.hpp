#pragma once

#include <memory>
#include <string>

namespace rationalstudio {

    enum class AngleMode {
        Radians,
        Degrees
    };

    // A compiled mathematical expression.
    //
    // Expressions are parsed once and can then be
    // evaluated repeatedly.
    //
    // This is particularly useful when graphing,
    // because a graph requires many evaluations
    // of the same mathematical expression.

    class Expression {
    public:

        explicit Expression(const std::string& source);

        ~Expression();

        Expression(Expression&&) noexcept;

        Expression& operator=(Expression&&) noexcept;

        Expression(const Expression&) = delete;

        Expression& operator=(const Expression&) = delete;

        bool valid() const noexcept;

        const std::string& error() const noexcept;

        bool dependsOnX() const noexcept;

        double evaluate(
            double x = 0,
            double ans = 0,
            AngleMode angle = AngleMode::Radians
        ) const noexcept;

    private:

        struct Impl;

        std::unique_ptr<Impl> impl_;
    };

} // namespace rationalstudio
