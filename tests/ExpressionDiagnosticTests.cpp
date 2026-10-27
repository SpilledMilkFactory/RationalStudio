#include "Expression.hpp"
#include "MathExpressionInput.hpp"
#include <iostream>
#include <string>
#include <utility>

using namespace rationalstudio;
namespace {
int checks = 0, failures = 0;
void check(bool condition, const std::string& title) {
    ++checks;
    if (!condition) { ++failures; std::cerr << "FAIL: " << title << '\n'; }
}
void expect(const std::string& source, ExpressionErrorKind kind,
            std::size_t offset, std::size_t length) {
    Expression expression(source);
    const auto* diagnostic = expression.diagnostic();
    check(!expression.valid(), source + " rejected");
    check(diagnostic != nullptr, source + " structured diagnostic");
    if (diagnostic) {
        check(diagnostic->kind == kind, source + " correct category");
        check(diagnostic->offset == offset, source + " correct offset");
        check(diagnostic->length == length, source + " correct extent");
        check(!diagnostic->message.empty(), source + " readable message");
    }
    check(!expression.error().empty(), source + " legacy error remains");
}
}
int main() {
    Expression valid("sin(pi/2)+x^2");
    check(valid.valid(), "valid AST retained");
    check(valid.diagnostic() == nullptr, "valid AST has no diagnostic");
    expect("2+*", ExpressionErrorKind::Syntax, 2, 1);
    expect("2+", ExpressionErrorKind::Syntax, 2, 0);
    expect("sin(x", ExpressionErrorKind::Syntax, 5, 0);
    expect("oops", ExpressionErrorKind::UnknownIdentifier, 0, 4);
    expect("2@3", ExpressionErrorKind::Lexical, 1, 1);
    expect("3e9999", ExpressionErrorKind::Lexical, 0, 1);
    expect("sin", ExpressionErrorKind::FunctionCall, 0, 3);
    expect("max(1)", ExpressionErrorKind::Arity, 0, 3);
    expect("sqrt(", ExpressionErrorKind::Syntax, 5, 0);
    // Unicode source columns map to the original glyph, not expanded bytes.
    check(MathExpressionInput::sourceColumn("π+√(x²)", 0) == 1, "pi column");
    check(MathExpressionInput::sourceColumn("π+√(x²)", 2) == 2, "after pi column");
    check(MathExpressionInput::sourceColumn("π+√(x²)", 3) == 3, "radical column");
    check(MathExpressionInput::sourceColumn("π+√(x²)", 9) == 6, "square exponent column");
    check(MathExpressionInput::sourceColumn("π+√(x²)", 12) == 8, "final end column");
    // All normal positions in a longer expression remain within source bounds.
    for (int i = 0; i < 250; ++i) {
        const std::string s = std::to_string(i) + "+*";
        Expression e(s);
        check(!e.valid() && e.diagnostic() &&
              e.diagnostic()->kind == ExpressionErrorKind::Syntax &&
              e.diagnostic()->offset == s.size() - 1,
              "deterministic operator error positioning");
    }
    VariableScope scope;
    check(scope.declare("a"), "scalar declared");
    Expression scoped("a+x", scope);
    check(scoped.valid() && scoped.diagnostic() == nullptr, "scope unaffected");
    Expression missing("b+x", scope);
    check(!missing.valid() && missing.diagnostic() &&
          missing.diagnostic()->kind == ExpressionErrorKind::UnknownIdentifier &&
          missing.diagnostic()->offset == 0, "unknown scoped variable location");
    std::cout << (failures ? "FAIL: " : "PASS: ") << checks
              << " ExpressionDiagnostic assertions\n";
    return failures ? 1 : 0;
}
