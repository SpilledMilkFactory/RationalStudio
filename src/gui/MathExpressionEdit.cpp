#include "MathExpressionEdit.hpp"

#include <QKeyEvent>
#include <QStyle>

namespace rationalstudio {

    MathExpressionEdit::MathExpressionEdit(QWidget* parent) : QLineEdit(parent) {
        setProperty("syntaxState", "empty");
        setClearButtonEnabled(true);
        setToolTip("Ctrl+Z undo · Ctrl+C/V copy/paste · select text before ( or √ to wrap it");
    }

    void MathExpressionEdit::setSyntaxState(const char* state) {
        if (property("syntaxState").toString() == QString::fromLatin1(state)) return;
        setProperty("syntaxState", state);
        style()->unpolish(this);
        style()->polish(this);
        update();
    }

    void MathExpressionEdit::insertPaired(const QString& opening, const QString& closing) {
        const QString selected = selectedText();
        const int start = selectionStart() >= 0 ? selectionStart() : cursorPosition();
        insert(opening + selected + closing);
        setCursorPosition(start + opening.size() + selected.size());
        setFocus(Qt::OtherFocusReason);
    }

    void MathExpressionEdit::insertMathSymbol(const QString& symbol) {
        if (symbol == "√") {
            insertPaired(QString::fromUtf8("√("), ")");
        } else if (symbol == "(" ) {
            insertPaired("(", ")");
        } else if (symbol.endsWith('(')) {
            insertPaired(symbol, ")");
        } else if (symbol == "²" || symbol == "³") {
            const QString selected = selectedText();
            if (!selected.isEmpty()) {
                const int start = selectionStart();
                insert("(" + selected + ")" + symbol);
                setCursorPosition(start + selected.size() + 3);
            } else {
                insert(symbol);
            }
            setFocus(Qt::OtherFocusReason);
        } else {
            insert(symbol);
            setFocus(Qt::OtherFocusReason);
        }
    }

    void MathExpressionEdit::keyPressEvent(QKeyEvent* event) {
        // Honor modifier-driven native Qt editing shortcuts, including undo.
        if (event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) {
            QLineEdit::keyPressEvent(event);
            return;
        }
        if (event->text() == "(") {
            insertPaired("(", ")");
            event->accept();
            return;
        }
        if (event->text() == ")" && selectionStart() < 0 &&
            cursorPosition() < text().size() && text().at(cursorPosition()) == QChar(')')) {
            setCursorPosition(cursorPosition() + 1);
        event->accept();
        return;
            }
            QLineEdit::keyPressEvent(event);
    }

} // namespace rationalstudio
