#pragma once

#include "AdvancedMathInput.hpp"
#include "Expression.hpp"

#include <QWidget>
#include <QStringList>
#include <functional>

class QComboBox;
class QLineEdit;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTabWidget;

namespace rationalstudio {

    // Issue #53: explicit, structured Qt presentation over previously validated
    // mathematical core APIs. The basic scientific evaluator is not replaced.
    class AdvancedMathPage final : public QWidget {
        Q_OBJECT

    public:
        explicit AdvancedMathPage(std::function<AngleMode()> angleMode, QWidget* parent = nullptr);

    private:
        struct Panel {
            QWidget* page = nullptr;
            QPlainTextEdit* output = nullptr;
            QPushButton* run = nullptr;
        };
        Panel createPanel(const QString& title, const QString& instruction);
        QLineEdit* addInput(Panel& panel, const QString& label, const QString& initial,
                            const QString& objectName);
        QComboBox* addChoice(Panel& panel, const QString& label, const QStringList& choices,
                             const QString& objectName);
        void publish(Panel& panel, bool success, const QString& summary,
                     const QString& details = {});
        void setupComplex();
        void setupCalculus();
        void setupMatrix();
        void setupVectors();
        void setupStatistics();
        void setupRegression();

        std::function<AngleMode()> angleMode_;
        QTabWidget* modes_ = nullptr;
        ComplexValue complexAnswer_{0, 0};  // distinct from Scientific's real ans.
    };

} // namespace rationalstudio

