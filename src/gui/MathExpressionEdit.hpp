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

        // Graph-only shortcuts are explicitly enabled by MainWindow.
        // Scientific inputs retain the normal Enter-to-calculate behavior.
        void setGraphEditingMode(bool enabled) noexcept { graphEditingMode_ = enabled; }
        bool graphEditingMode() const noexcept { return graphEditingMode_; }

    signals:
        void graphNavigationRequested(int direction);
        void graphRowRequested();

    protected:
        void keyPressEvent(QKeyEvent* event) override;

    private:
        void insertPaired(const QString& opening, const QString& closing);
        bool graphEditingMode_ = false;
    };

} // namespace rationalstudio
