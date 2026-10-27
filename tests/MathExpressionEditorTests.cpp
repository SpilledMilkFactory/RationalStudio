#include "MainWindow.hpp"
#include "MathExpressionEdit.hpp"
#include "MathExpressionInput.hpp"

#include <QApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QTabWidget>
#include <QToolButton>

#include <iostream>

using namespace rationalstudio;
namespace { int checks=0, failures=0;
    void check(bool ok, const char* what) { ++checks; if (!ok) { ++failures; std::cerr << "FAIL: " << what << '\n'; } }
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MathExpressionEdit editor;
    editor.setText("x+3");
    editor.setCursorPosition(1);
    editor.insertMathSymbol("π");
    check(editor.text() == "xπ+3", "inserts symbol at caret");
    check(editor.cursorPosition() == 2, "caret follows inserted symbol");
    editor.setText("x+1");
    editor.setSelection(0, 3);
    editor.insertMathSymbol("√");
    check(editor.text() == "√(x+1)", "square root wraps selected expression");
    check(editor.cursorPosition() == 5, "caret after wrapped selection, within closing parenthesis");
    editor.setText("x");
    editor.selectAll();
    editor.insertMathSymbol("²");
    check(editor.text() == "(x)²", "superscript wraps selected input");
    editor.setText("sin");
    editor.setCursorPosition(3);
    QKeyEvent opening(QEvent::KeyPress, Qt::Key_ParenLeft, Qt::NoModifier, "(");
    QApplication::sendEvent(&editor, &opening);
    check(editor.text() == "sin()" && editor.cursorPosition() == 4,
          "typed parentheses pair and insert caret inside");
    QKeyEvent closing(QEvent::KeyPress, Qt::Key_ParenRight, Qt::NoModifier, ")");
    QApplication::sendEvent(&editor, &closing);
    check(editor.text() == "sin()" && editor.cursorPosition() == 5,
          "typing existing closing parenthesis skips duplicate");
    editor.setSyntaxState("invalid");
    check(editor.property("syntaxState").toString() == "invalid", "syntax feedback state");
    check(MathExpressionInput::normalize(editor.text().toStdString()) == "sin()", "ASCII retained");

    MainWindow window;
    window.resize(1250, 790);
    window.show();
    auto* tabs = window.findChild<QTabWidget*>();
    check(tabs && tabs->count() >= 2, "main graphing tab available");
    if(tabs) tabs->setCurrentIndex(1);
    app.processEvents();
    auto* scientific = window.findChild<MathExpressionEdit*>("ScientificExpression");
    check(scientific != nullptr, "scientific editor integrated");
    if (scientific) {
        scientific->setText("√(9)+π");
        check(scientific->property("syntaxState").toString() == "valid",
              "scientific live parse accepts mathematical symbols");
        scientific->setText("2+");
        check(scientific->property("syntaxState").toString() == "incomplete",
              "scientific incomplete syntax feedback");
    }
    auto graphRows = window.findChildren<MathExpressionEdit*>();
    MathExpressionEdit* graphEditor=nullptr;
    for (auto* candidate : graphRows) {
        if (candidate->objectName().startsWith("GraphExpression_")) {
            graphEditor=candidate;
            break;
        }
    }
    check(graphEditor != nullptr, "graph editor integrated");
    if (graphEditor) {
        graphEditor->setText("y = x²+π");
        check(graphEditor->property("syntaxState").toString() == "valid",
              "immediate graph valid state on input change");
        graphEditor->setText("y = 2+*");
        check(graphEditor->property("syntaxState").toString() == "invalid",
              "immediate graph syntax error without waiting for repaint");
        graphEditor->setText("y = x+");
        check(graphEditor->property("syntaxState").toString() == "incomplete",
              "immediate incomplete graph expression");
        graphEditor->setText("y = x");
        graphEditor->setCursorPosition(graphEditor->text().size());
        auto* pi = window.findChild<QToolButton*>("graphMathSymbolPi");
        check(pi != nullptr, "graph pi button integrated");
        if (pi) {
            pi->click();
            check(graphEditor->text().endsWith("π"), "symbol button uses active graph cursor");
        }
    }
    std::cout << (failures?"FAIL: ":"PASS: ") << checks
    << " MathExpressionEditor assertions\n";
    return failures?1:0;
}
