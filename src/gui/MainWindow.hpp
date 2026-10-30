#pragma once

#include "Expression.hpp"
#include "FunctionDefinitions.hpp"
#include "GraphCanvas.hpp"

#include <QMainWindow>
#include <QColor>
#include <QString>
#include <memory>
#include <limits>
#include <vector>

class QButtonGroup;
class QComboBox;
class QDoubleSpinBox;
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

class VariableSliderPanel;
class MathExpressionEdit;

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    // File-based entry points for deterministic GUI integration tests and
    // Save/Open actions. No modal dialogs on failure.
    bool saveWorkspace(const QString& path, QString* error = nullptr) const;
    bool loadWorkspace(const QString& path, QString* error = nullptr);
    bool defineWorkspaceFunction(const QString& statement, QString* error = nullptr);

private:
    struct GraphRow {
        QWidget* widget = nullptr;
        MathExpressionEdit* input = nullptr;
        MathExpressionEdit* yInput = nullptr; // Parametric second coordinate.
        QDoubleSpinBox* tMinimum = nullptr;
        QDoubleSpinBox* tMaximum = nullptr;
        bool parametric = false;
        bool polar = false;
        bool implicit = false;
        QString parsedYSource;
        double compiledMinimum = std::numeric_limits<double>::quiet_NaN();
        double compiledMaximum = std::numeric_limits<double>::quiet_NaN();
        std::shared_ptr<const Expression> parsedYExpression;
        QToolButton* visible = nullptr;
        QLabel* info = nullptr;
        QColor color;
        QString parsedSource;
        std::shared_ptr<const Expression> parsedExpression;
    };

    QWidget* makeScientificPage();
    QWidget* makeGraphPage();
    QWidget* makeRationalPage();
    void appendScientificText(const QString& text);
    void calculateScientific();
    void updateScientificPreview();
    void calculateFraction();
    void addGraphRow(const QString& text = QString(), bool visible = true, QColor color = QColor());
    void addParametricRow(const QString& x = "x(t) = cos(t)",
                          const QString& y = "y(t) = sin(t)",
                          double minimum = 0, double maximum = 6.283185307179586,
                          bool visible = true, QColor color = QColor());
    void addPolarRow(const QString& expression = "r = 2cos(θ)",
                     double minimum = 0, double maximum = 6.283185307179586,
                     bool visible = true, QColor color = QColor());
    void addImplicitRow(const QString& equation = "x² + y² = 25",
                        bool visible = true, QColor color = QColor());
    void removeGraphRow(GraphRow* row);
    void rebuildGraph(const std::vector<std::string>& changedScope = {});
    void updateGraphEditorFeedback(GraphRow* row);
    void onVariableDefinitionsChanged(const std::vector<std::string>& changed,
                                      bool scopeChanged);
    void saveGraph();
    void loadGraph();
    void exportImage();
    void setAngleMode(AngleMode mode);
    void graphScientificExpression();

    QTabWidget* tabs_ = nullptr;
    QPushButton* radiansButton_ = nullptr;
    QPushButton* degreesButton_ = nullptr;
    MathExpressionEdit* scientificInput_ = nullptr;
    QLabel* scientificResult_ = nullptr;
    QLabel* scientificHint_ = nullptr;
    QListWidget* history_ = nullptr;
    QLineEdit* fractionA_ = nullptr;
    QLineEdit* fractionB_ = nullptr;
    QComboBox* fractionOp_ = nullptr;
    QLabel* fractionResult_ = nullptr;
    QLabel* fractionDecimal_ = nullptr;
    GraphCanvas* graph_ = nullptr;
    VariableSliderPanel* variableSliders_ = nullptr;
    FunctionDefinitions functionDefinitions_;
    MathExpressionEdit* activeGraphInput_ = nullptr;
    QVBoxLayout* graphList_ = nullptr;
    QLabel* graphStatus_ = nullptr;
    QTimer* graphDebounce_ = nullptr;
    std::vector<GraphRow*> graphRows_;
    AngleMode angleMode_ = AngleMode::Radians;
    double lastAnswer_ = 0;
    unsigned nextColor_ = 0;
};

} // namespace rationalstudio
