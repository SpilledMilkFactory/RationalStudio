#pragma once

#include "Expression.hpp"
#include "GraphCanvas.hpp"

#include <QMainWindow>
#include <QColor>

#include <vector>

class QButtonGroup;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QScrollArea;
class QTabWidget;
class QTimer;
class QVBoxLayout;
class QToolButton;

namespace rationalstudio {

    class MainWindow final : public QMainWindow {
    public:

        explicit MainWindow(QWidget* parent = nullptr);

    private:

        struct GraphRow {

            QWidget* widget = nullptr;

            QLineEdit* input = nullptr;

            QToolButton* visible = nullptr;

            QLabel* info = nullptr;

            QColor color;
        };

        QWidget* makeScientificPage();

        QWidget* makeGraphPage();

        QWidget* makeRationalPage();

        void appendScientificText(const QString& text);

        void calculateScientific();

        void updateScientificPreview();

        void calculateFraction();

        void addGraphRow(
            const QString& text = QString(),
                         bool visible = true,
                         QColor color = QColor()
        );

        void removeGraphRow(GraphRow* row);

        void rebuildGraph();

        void saveGraph();

        void loadGraph();

        void exportImage();

        void setAngleMode(AngleMode mode);

        void graphScientificExpression();

        QTabWidget* tabs_ = nullptr;

        QPushButton* radiansButton_ = nullptr;
        QPushButton* degreesButton_ = nullptr;

        QLineEdit* scientificInput_ = nullptr;

        QLabel* scientificResult_ = nullptr;
        QLabel* scientificHint_ = nullptr;

        QListWidget* history_ = nullptr;

        QLineEdit* fractionA_ = nullptr;
        QLineEdit* fractionB_ = nullptr;

        QComboBox* fractionOp_ = nullptr;

        QLabel* fractionResult_ = nullptr;
        QLabel* fractionDecimal_ = nullptr;

        GraphCanvas* graph_ = nullptr;

        QVBoxLayout* graphList_ = nullptr;

        QLabel* graphStatus_ = nullptr;

        QTimer* graphDebounce_ = nullptr;

        std::vector<GraphRow*> graphRows_;

        AngleMode angleMode_ = AngleMode::Radians;

        double lastAnswer_ = 0;

        unsigned nextColor_ = 0;
    };

} // namespace rationalstudio
