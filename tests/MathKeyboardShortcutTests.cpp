#include "MainWindow.hpp"
#include "MathExpressionEdit.hpp"
#include "MathExpressionInput.hpp"
#include "Expression.hpp"

#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QTabWidget>
#include <QWidget>

#include <cmath>
#include <iostream>
#include <vector>

using namespace rationalstudio;

namespace {
int checks = 0;
int failures = 0;
void check(bool condition, const char* description) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}
void key(QWidget* widget, Qt::Key code, Qt::KeyboardModifiers mods,
         const QString& text = {}) {
    QKeyEvent event(QEvent::KeyPress, code, mods, text);
    QApplication::sendEvent(widget, &event);
}
std::vector<MathExpressionEdit*> graphEditors(MainWindow& window) {
    std::vector<MathExpressionEdit*> result;
    for (auto* editor : window.findChildren<MathExpressionEdit*>()) {
        if (editor->objectName().startsWith("GraphExpression_"))
            result.push_back(editor);
    }
    // Child discovery order is stable for rows appended by MainWindow.
    return result;
}
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    MathExpressionEdit editor;
    editor.setText("x+3");
    editor.setCursorPosition(1);
    key(&editor, Qt::Key_P, Qt::AltModifier);
    check(editor.text() == QString::fromUtf8("xπ+3"), "Alt+P inserts pi at cursor");
    check(editor.cursorPosition() == 2, "pi insertion preserves cursor position");
    key(&editor, Qt::Key_Z, Qt::ControlModifier);
    check(editor.text() == "x+3", "Ctrl+Z remains native undo");
    key(&editor, Qt::Key_Y, Qt::ControlModifier);
    // Qt redo may be Ctrl+Y or Ctrl+Shift+Z depending on platform.
    check(editor.text() == "x+3" || editor.text() == QString::fromUtf8("xπ+3"),
          "platform redo does not corrupt contents");

    editor.setText("x+1");
    editor.setSelection(0, 3);
    key(&editor, Qt::Key_R, Qt::AltModifier);
    check(editor.text() == QString::fromUtf8("√(x+1)"), "Alt+R wraps selection in root");
    check(editor.cursorPosition() == 5, "root shortcut caret position");

    editor.setText("x");
    editor.selectAll();
    key(&editor, Qt::Key_2, Qt::AltModifier);
    check(editor.text() == QString::fromUtf8("(x)²"), "Alt+2 wraps selected power");
    editor.setText("x");
    editor.setCursorPosition(1);
    key(&editor, Qt::Key_3, Qt::AltModifier);
    check(editor.text() == QString::fromUtf8("x³"), "Alt+3 inserts cubic exponent");

    editor.setText("x+1");
    editor.selectAll();
    key(&editor, Qt::Key_S, Qt::AltModifier);
    check(editor.text() == "sin(x+1)", "Alt+S wraps selection in sine");
    check(editor.cursorPosition() == 7, "function wrapper maintains caret");
    editor.setText("x");
    editor.selectAll();
    key(&editor, Qt::Key_C, Qt::AltModifier);
    check(editor.text() == "cos(x)", "Alt+C wraps selection in cosine");
    editor.setText("x");
    editor.selectAll();
    key(&editor, Qt::Key_T, Qt::AltModifier);
    check(editor.text() == "tan(x)", "Alt+T wraps selection in tangent");

    editor.setText("3");
    editor.setCursorPosition(1);
    key(&editor, Qt::Key_M, Qt::AltModifier);
    editor.insert("4");
    key(&editor, Qt::Key_D, Qt::AltModifier);
    editor.insert("2");
    check(editor.text() == QString::fromUtf8("3×4÷2"), "Alt+M/D math operators");
    Expression compiled(MathExpressionInput::normalize(editor.text().toStdString()));
    check(compiled.valid(), "keyboard mathematical expression compiles");
    if (compiled.valid())
        check(std::abs(compiled.evaluate(0, 0, AngleMode::Radians) - 6.0) < 1e-12,
              "keyboard mathematical expression evaluates correctly");

    editor.setText("sin");
    editor.setCursorPosition(3);
    key(&editor, Qt::Key_ParenLeft, Qt::NoModifier, "(");
    check(editor.text() == "sin()" && editor.cursorPosition() == 4,
          "ordinary typed parentheses still auto-pair");
    key(&editor, Qt::Key_ParenRight, Qt::NoModifier, ")");
    check(editor.text() == "sin()" && editor.cursorPosition() == 5,
          "ordinary typed closing parenthesis still skips duplicate");

    editor.setText("abc");
    editor.setReadOnly(true);
    key(&editor, Qt::Key_P, Qt::AltModifier);
    check(editor.text() == "abc", "math shortcuts honor read-only editor");
    editor.setReadOnly(false);
    editor.setText("abc");
    key(&editor, Qt::Key_A, Qt::ControlModifier);
    check(editor.hasSelectedText() && editor.selectedText() == "abc",
          "Ctrl+A standard selection is preserved");

    MainWindow window;
    window.resize(1250, 790);
    window.show();
    QApplication::processEvents();
    auto* scientific = window.findChild<MathExpressionEdit*>("ScientificExpression");
    check(scientific != nullptr, "scientific editor found in actual MainWindow");
    if (scientific) {
        scientific->setText("sin");
        scientific->setCursorPosition(3);
        key(scientific, Qt::Key_S, Qt::AltModifier);
        check(scientific->text() == "sinsin()", "scientific keyboard function insertion");
        scientific->setText("2");
        key(scientific, Qt::Key_P, Qt::AltModifier);
        check(scientific->text() == QString::fromUtf8("2π"),
              "scientific mathematical constant insertion");
        check(scientific->property("syntaxState").toString() == "valid",
              "scientific parser feedback remains immediate");
    }

    auto* tabs = window.findChild<QTabWidget*>();
    check(tabs && tabs->count() >= 2, "graphing workspace available");
    if (tabs) tabs->setCurrentIndex(1);
    QApplication::processEvents();
    auto rows = graphEditors(window);
    check(rows.size() == 2, "two baseline graph rows exist");
    if (rows.size() >= 2) {
        auto* first = rows.front();
        auto* second = rows[1];
        first->setFocus();
        key(first, Qt::Key_Down, Qt::AltModifier);
        check(QApplication::focusWidget() == second,
              "Alt+Down focuses next graph row");
        key(second, Qt::Key_Up, Qt::AltModifier);
        check(QApplication::focusWidget() == first,
              "Alt+Up focuses preceding graph row");
        key(first, Qt::Key_Up, Qt::AltModifier);
        check(QApplication::focusWidget() == second,
              "graph row navigation wraps at first row");
        key(second, Qt::Key_Down, Qt::AltModifier);
        check(QApplication::focusWidget() == first,
              "graph row navigation wraps at last row");
        first->setText("y = 2");
        first->setCursorPosition(first->text().size());
        key(first, Qt::Key_P, Qt::AltModifier);
        check(first->text() == QString::fromUtf8("y = 2π"),
              "graph shortcut modifies correct editor");
        check(first->property("syntaxState").toString() == "valid",
              "graph feedback remains valid after math shortcut");
        key(first, Qt::Key_Return, Qt::ControlModifier);
        auto expanded = graphEditors(window);
        check(expanded.size() == 3, "Ctrl+Enter adds exactly one graph row");
        if (expanded.size() == 3) {
            check(QApplication::focusWidget() == expanded.back(),
                  "new graph row gains keyboard focus");
            expanded.back()->setText("y = sin(x)");
            check(expanded.back()->property("syntaxState").toString() == "valid",
                  "new row is ready for graph editing");
            key(expanded.back(), Qt::Key_Up, Qt::AltModifier);
            check(QApplication::focusWidget() == second,
                  "new graph row participates in keyboard navigation");
        }
    }
    std::cout << (failures ? "FAIL: " : "PASS: ") << checks
              << " MathKeyboardShortcut assertions\n";
    return failures ? 1 : 0;
}
