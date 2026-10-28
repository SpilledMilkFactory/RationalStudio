#include "MathExpressionEdit.hpp"

#include <QKeyEvent>
#include <QStyle>

namespace rationalstudio {

    MathExpressionEdit::MathExpressionEdit(QWidget* parent) : QLineEdit(parent) {
        setProperty("syntaxState", "empty");
        setClearButtonEnabled(true);
        setToolTip("Alt+P π · Alt+R √ · Alt+2 ² · Alt+3 ³ · Alt+S sin · Alt+C cos · Alt+T tan · Alt+M × · Alt+D ÷ · Ctrl+Z undo");
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
        if (graphEditingMode_ && event->modifiers() == Qt::AltModifier) {
            if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) {
                emit graphNavigationRequested(event->key() == Qt::Key_Up ? -1 : 1);
                event->accept();
                return;
            }
        }
        if (graphEditingMode_ && event->modifiers() == Qt::ControlModifier &&
            (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
            emit graphRowRequested();
            event->accept();
            return;
        }

        // Alt-only math shortcuts are intentionally editor-local. Standard
        // Ctrl/Cmd/Shift editing shortcuts remain handled by QLineEdit.
        if (event->modifiers() == Qt::AltModifier && !isReadOnly()) {
            QString insertion;
            switch (event->key()) {
                case Qt::Key_P: insertion = QString::fromUtf8("π"); break;
                case Qt::Key_R: insertion = QString::fromUtf8("√"); break;
                case Qt::Key_2: insertion = QString::fromUtf8("²"); break;
                case Qt::Key_3: insertion = QString::fromUtf8("³"); break;
                case Qt::Key_M: insertion = QString::fromUtf8("×"); break;
                case Qt::Key_D: insertion = QString::fromUtf8("÷"); break;
                case Qt::Key_S: insertion = "sin("; break;
                case Qt::Key_C: insertion = "cos("; break;
                case Qt::Key_T: insertion = "tan("; break;
                default: break;
            }
            if (!insertion.isEmpty()) {
                insertMathSymbol(insertion);
                event->accept();
                return;
            }
        }

        // Leave native editing commands (Ctrl+Z/C/V/X/A, redo, arrow keys,
        // deletion and platform-specific commands) to QLineEdit.
        if (event->modifiers() != Qt::NoModifier) {
            QLineEdit::keyPressEvent(event);
            return;
        }
        if (event->text() == "(" && !isReadOnly()) {
            insertPaired("(", ")");
            event->accept();
            return;
        }
        if (event->text() == ")" && selectionStart() < 0 &&
            cursorPosition() < text().size() &&
            text().at(cursorPosition()) == QChar(')')) {
            setCursorPosition(cursorPosition() + 1);
            event->accept();
            return;
        }
        QLineEdit::keyPressEvent(event);
    }

} // namespace rationalstudio
