#pragma once

#include <QLineEdit>
#include <QString>

class QKeyEvent;

namespace rationalstudio {

    // Enhanced single-line editor without changing the expression parser or
    // overriding ordinary Qt cursor navigation, clipboard, shortcuts and undo.
    class MathExpressionEdit final : public QLineEdit {
        Q_OBJECT

    public:
        explicit MathExpressionEdit(QWidget* parent = nullptr);

        // Inserts presentation glyphs at the caret, replacing selected text;
        // roots/parentheses wrap selections, with the caret placed sensibly.
        void insertMathSymbol(const QString& symbol);
        void setSyntaxState(const char* state);

    protected:
        void keyPressEvent(QKeyEvent* event) override;

    private:
        void insertPaired(const QString& opening, const QString& closing);
    };

} // namespace rationalstudio
