#include "AdvancedMathPage.hpp"

#include "ComplexMath.hpp"
#include "NumericalDifferentiation.hpp"
#include "NumericalIntegration.hpp"
#include "NumericalRootFinding.hpp"
#include "MatrixMath.hpp"
#include "DescriptiveStatistics.hpp"
#include "StatisticalRegression.hpp"

#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QStringList>
#include <QSizePolicy>
#include <utility>

#include <cmath>
#include <limits>
#include <optional>
#include <vector>

namespace rationalstudio {
namespace {

QString number(double n) {
    return std::isfinite(n) ? QString::number(n, 'g', 14) : QStringLiteral("undefined");
}
std::optional<double> finiteNumber(QLineEdit* edit) {
    bool ok = false;
    const double value = edit->text().trimmed().toDouble(&ok);
    if (!ok || !std::isfinite(value)) return std::nullopt;
    return value;
}
QString formatVector(const MathVector& vector) {
    QStringList values;
    for (const double value : vector.values) values.append(number(value));
    return "[" + values.join(", ") + "]";
}
QString formatMatrix(const Matrix& matrix) {
    QStringList rows;
    for (std::size_t row = 0; row < matrix.rows; ++row) {
        QStringList values;
        for (std::size_t col = 0; col < matrix.columns; ++col)
            values.append(number(matrix.at(row, col)));
        rows.append("[" + values.join(", ") + "]");
    }
    return rows.join("\n");
}
QLineEdit* line(const QString& name, const QString& text) {
    auto* input = new QLineEdit(text);
    input->setObjectName(name);
    input->setAccessibleName(name);
    input->setClearButtonEnabled(true);
    return input;
}
QString errorText(const std::string& error) { return QString::fromStdString(error); }

} // namespace

AdvancedMathPage::AdvancedMathPage(std::function<AngleMode()> mode, QWidget* parent)
    : QWidget(parent), angleMode_(std::move(mode)) {
    setObjectName("advancedMathPage");
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(18, 12, 18, 14);
    auto* title = new QLabel("Advanced mathematics");
    title->setStyleSheet("font-size:23px; font-weight:bold;");
    outer->addWidget(title);
    auto* help = new QLabel("Separate workspaces for complex values, numerical calculus, matrices, statistics, and regression. "
                            "Results are approximate and report errors without changing the basic calculator's ans.");
    help->setWordWrap(true);
    help->setStyleSheet("color:#9aadc5;");
    outer->addWidget(help);
    modes_ = new QTabWidget(this);
    modes_->setObjectName("advancedMathTabs");
    outer->addWidget(modes_, 1);
    setupComplex();
    setupCalculus();
    setupMatrix();
    setupVectors();
    setupStatistics();
    setupRegression();
}

AdvancedMathPage::Panel AdvancedMathPage::createPanel(const QString& title,
                                                      const QString& instruction) {
    Panel panel;
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    panel.page = new QWidget;
    panel.page->setObjectName("advanced" + QString(title).remove(' '));
    auto* layout = new QVBoxLayout(panel.page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);
    auto* hint = new QLabel(instruction);
    hint->setWordWrap(true);
    hint->setTextFormat(Qt::PlainText);
    hint->setStyleSheet("color:#a8c1d4;");
    layout->addWidget(hint);
    // Inputs are direct children of the page; sections remain readable in narrow windows.
    panel.run = new QPushButton("Calculate", panel.page);
    panel.run->setMinimumHeight(42);
    panel.run->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    panel.output = new QPlainTextEdit(panel.page);
    panel.output->setObjectName("advanced" + QString(title).remove(' ') + "Output");
    panel.output->setReadOnly(true);
    panel.output->setPlainText("Choose an operation and select Calculate.");
    panel.output->setMinimumHeight(155);
    panel.output->setAccessibleName(title + " calculation result");
    scroll->setWidget(panel.page);
    modes_->addTab(scroll, title);
    return panel;
}

QLineEdit* AdvancedMathPage::addInput(Panel& panel, const QString& label,
                                     const QString& initial, const QString& name) {
    auto* text = new QLabel(label, panel.page);
    auto* input = line(name, initial);
    auto* layout = static_cast<QVBoxLayout*>(panel.page->layout());
    layout->addWidget(text);
    layout->addWidget(input);
    return input;
}

QComboBox* AdvancedMathPage::addChoice(Panel& panel, const QString& label,
                                      const QStringList& options, const QString& name) {
    auto* layout = static_cast<QVBoxLayout*>(panel.page->layout());
    layout->addWidget(new QLabel(label));
    auto* choice = new QComboBox(panel.page);
    choice->setObjectName(name);
    choice->setAccessibleName(label);
    choice->addItems(options);
    layout->addWidget(choice);
    return choice;
}

void AdvancedMathPage::publish(Panel& panel, bool success, const QString& summary,
                               const QString& details) {
    panel.output->setPlainText((success ? "Result: " : "Error: ") + summary +
                                (details.isEmpty() ? QString() : "\n\n" + details));
    panel.output->setProperty("calculationSuccess", success);
}

void AdvancedMathPage::setupComplex() {
    auto panel = createPanel("Complex", "Enter a complex expression, for example sqrt(-1), 3+2i or exp(i*pi). "
                          "Principal branches are used; complex ans is separate from real ans.");
    auto* expression = addInput(panel, "Complex expression", "sqrt(-1)", "advancedComplexExpression");
    auto* layout = static_cast<QVBoxLayout*>(panel.page->layout());
    panel.run->setObjectName("advancedComplexRun");
    layout->addWidget(panel.run);
    layout->addWidget(panel.output, 1);
    connect(expression, &QLineEdit::returnPressed, panel.run, &QPushButton::click);
    connect(panel.run, &QPushButton::clicked, this, [this, panel, expression]() mutable {
        ComplexExpression parsed(expression->text().trimmed().toStdString());
        if (!parsed.valid()) { publish(panel, false, errorText(parsed.error())); return; }
        const auto result = parsed.evaluate({0, 0}, complexAnswer_, angleMode_());
        if (!result.successful()) {
            publish(panel, false, QString::fromLatin1(complexStatusName(result.status)) +
                                 (result.message.empty() ? QString() : ": " + errorText(result.message)));
            return;
        }
        complexAnswer_ = result.value;
        const AdvancedMathValue typed = result.value;
        publish(panel, true, errorText(formatComplex(std::get<ComplexValue>(typed))),
                "Type: complex · principal branch · IEEE-754 double precision");
    });
}

void AdvancedMathPage::setupCalculus() {
    auto panel = createPanel("Calculus", "Use a real expression in x. Derivative uses X; integral and bracketed root use A and B. "
                          "All computations are bounded numerical estimates, not symbolic proofs.");
    auto* mode = addChoice(panel, "Operation", {"Derivative f'(x)", "Definite integral", "Root in [A, B]"},
                           "advancedCalculusOperation");
    auto* expr = addInput(panel, "Function f(x)", "x^2", "advancedCalculusExpression");
    auto* x = addInput(panel, "X (derivative only)", "2", "advancedCalculusX");
    auto* a = addInput(panel, "A (integral/root lower bound)", "0", "advancedCalculusA");
    auto* b = addInput(panel, "B (integral/root upper bound)", "3", "advancedCalculusB");
    auto* layout = static_cast<QVBoxLayout*>(panel.page->layout());
    panel.run->setObjectName("advancedCalculusRun");
    layout->addWidget(panel.run);
    layout->addWidget(panel.output, 1);
    connect(panel.run, &QPushButton::clicked, this, [this, panel, mode, expr, x, a, b]() mutable {
        Expression parsed(expr->text().trimmed().toStdString());
        if (!parsed.valid()) { publish(panel, false, errorText(parsed.error())); return; }
        const VariableBindings bindings;
        const int kind = mode->currentIndex();
        if (kind == 0) {
            const auto coordinate = finiteNumber(x);
            if (!coordinate) { publish(panel, false, "X must be a finite number"); return; }
            const auto result = differentiate(parsed, *coordinate, angleMode_(), bindings);
            if (!result.converged()) {
                publish(panel, false, QString::fromLatin1(derivativeStatusName(result.status)));
                return;
            }
            const AdvancedMathValue typed = result.value;
            publish(panel, true, number(std::get<double>(typed)),
                    QString("Method: %1\nEstimated error: %2\nEvaluations: %3\nRefinements: %4")
                        .arg(derivativeMethodName(result.method)).arg(number(result.estimatedError))
                        .arg(static_cast<qulonglong>(result.evaluations))
                        .arg(static_cast<qulonglong>(result.refinements)));
        } else {
            const auto low = finiteNumber(a), high = finiteNumber(b);
            if (!low || !high) { publish(panel, false, "A and B must be finite numbers"); return; }
            if (kind == 1) {
                const auto result = integrate(parsed, *low, *high, angleMode_(), bindings);
                if (!result.converged()) {
                    publish(panel, false, QString::fromLatin1(integrationStatusName(result.status)));
                    return;
                }
                publish(panel, true, number(result.value),
                        QString("Method: adaptive Gauss-Kronrod 7/15\nEstimated error: %1\nEvaluations: %2\nSubdivisions: %3")
                            .arg(number(result.estimatedError))
                            .arg(static_cast<qulonglong>(result.evaluations))
                            .arg(static_cast<qulonglong>(result.subdivisions)));
            } else {
                const auto result = solveBracketedRoot(parsed, *low, *high, angleMode_(), bindings);
                if (!result.converged()) {
                    publish(panel, false, QString::fromLatin1(rootStatusName(result.status)));
                    return;
                }
                publish(panel, true, number(result.x),
                        QString("Residual: %1\nBracket: [%2, %3]\nEvaluations: %4\nIterations: %5")
                            .arg(number(result.residual)).arg(number(result.bracketLower))
                            .arg(number(result.bracketUpper))
                            .arg(static_cast<qulonglong>(result.evaluations))
                            .arg(static_cast<qulonglong>(result.iterations)));
            }
        }
    });
}

void AdvancedMathPage::setupMatrix() {
    auto panel = createPanel("Matrices", "Rows are separated by semicolons, entries by commas. "
                          "Example A: 1,2;3,4. For linear solve, B is the RHS vector (for example 5,11).");
    auto* mode = addChoice(panel, "Operation", {"Transpose A", "Determinant A", "A + B", "A - B", "A × B", "Scale A by B (scalar)", "Solve A × x = b"},
                           "advancedMatrixOperation");
    auto* left = addInput(panel, "Matrix A", "1,2;3,4", "advancedMatrixA");
    auto* right = addInput(panel, "Matrix B / RHS vector", "5,6;7,8", "advancedMatrixB");
    auto* layout = static_cast<QVBoxLayout*>(panel.page->layout());
    panel.run->setObjectName("advancedMatrixRun");
    layout->addWidget(panel.run);
    layout->addWidget(panel.output, 1);
    connect(panel.run, &QPushButton::clicked, this, [this, panel, mode, left, right]() mutable {
        const auto a = parseAdvancedMatrix(left->text().toStdString());
        if (!a.valid()) { publish(panel, false, "A: " + errorText(a.error)); return; }
        const int op = mode->currentIndex();
        if (op == 1) {
            const auto result = determinant(a.value);
            if (!result.successful()) { publish(panel, false, matrixStatusName(result.status)); return; }
            publish(panel, true, number(result.value), "Type: real scalar · LU numerical determinant");
        } else if (op == 0) {
            const auto result = transposeMatrix(a.value);
            if (!result.successful()) { publish(panel, false, matrixStatusName(result.status)); return; }
            const AdvancedMathValue typed = result.value;
            publish(panel, true, formatMatrix(std::get<Matrix>(typed)), "Type: real matrix");
        } else if (op == 6) {
            const auto rhs = parseAdvancedNumbers(right->text().toStdString(), 128);
            if (!rhs.valid()) { publish(panel, false, "b: " + errorText(rhs.error)); return; }
            const auto vector = makeVector(rhs.values);
            if (!vector.successful()) { publish(panel, false, matrixStatusName(vector.status)); return; }
            const auto result = solveLinearSystem(a.value, vector.value);
            if (!result.successful()) { publish(panel, false, matrixStatusName(result.status)); return; }
            publish(panel, true, formatVector(result.solution),
                    "Type: real vector\nRelative residual: " + number(result.relativeResidual) +
                    "\nReciprocal condition estimate: " + number(result.reciprocalCondition));
        } else if (op == 5) {
            const auto scalar = finiteNumber(right);
            if (!scalar) { publish(panel, false, "B must be a finite scalar"); return; }
            const auto result = scaleMatrix(a.value, *scalar);
            if (!result.successful()) { publish(panel, false, matrixStatusName(result.status)); return; }
            publish(panel, true, formatMatrix(result.value), "Type: real matrix");
        } else {
            const auto b = parseAdvancedMatrix(right->text().toStdString());
            if (!b.valid()) { publish(panel, false, "B: " + errorText(b.error)); return; }
            const auto result = op == 2 ? addMatrices(a.value, b.value)
                              : op == 3 ? subtractMatrices(a.value, b.value)
                                        : multiplyMatrices(a.value, b.value);
            if (!result.successful()) { publish(panel, false, matrixStatusName(result.status)); return; }
            publish(panel, true, formatMatrix(result.value), "Type: real matrix");
        }
    });
}

void AdvancedMathPage::setupVectors() {
    auto panel = createPanel("Vectors", "Vectors use comma-separated finite entries. "
                            "For matrix × vector, A is a semicolon-separated matrix and B is the vector.");
    auto* mode = addChoice(panel, "Operation",
                           {"A + B", "A - B", "Dot(A, B)", "Norm(A)", "Scale A by scalar", "Matrix A × vector B"},
                           "advancedVectorOperation");
    auto* left = addInput(panel, "Vector A / Matrix A", "1,2,3", "advancedVectorA");
    auto* right = addInput(panel, "Vector B / Scale", "4,5,6", "advancedVectorB");
    auto* layout = static_cast<QVBoxLayout*>(panel.page->layout());
    panel.run->setObjectName("advancedVectorRun");
    layout->addWidget(panel.run);
    layout->addWidget(panel.output, 1);
    connect(panel.run, &QPushButton::clicked, this, [this, panel, mode, left, right]() mutable {
        const int op = mode->currentIndex();
        if (op == 5) {
            const auto m = parseAdvancedMatrix(left->text().toStdString());
            const auto b = parseAdvancedNumbers(right->text().toStdString(), 128);
            if (!m.valid() || !b.valid()) {
                publish(panel, false, !m.valid() ? errorText(m.error) : errorText(b.error)); return;
            }
            const auto vector = makeVector(b.values);
            if (!vector.successful()) { publish(panel, false, matrixStatusName(vector.status)); return; }
            const auto product = multiplyMatrixVector(m.value, vector.value);
            if (!product.successful()) { publish(panel, false, matrixStatusName(product.status)); return; }
            publish(panel, true, formatVector(product.value), "Type: real vector");
            return;
        }
        const auto a = parseAdvancedNumbers(left->text().toStdString(), 128);
        if (!a.valid()) { publish(panel, false, "A: " + errorText(a.error)); return; }
        const auto first = makeVector(a.values);
        if (!first.successful()) { publish(panel, false, matrixStatusName(first.status)); return; }
        if (op == 3) {
            const auto norm = normVector(first.value);
            if (!norm.successful()) { publish(panel, false, matrixStatusName(norm.status)); return; }
            publish(panel, true, number(norm.value), "Type: real scalar · Euclidean norm");
            return;
        }
        if (op == 4) {
            const auto scalar = finiteNumber(right);
            if (!scalar) { publish(panel, false, "Scale must be finite"); return; }
            const auto scaled = scaleVector(first.value, *scalar);
            if (!scaled.successful()) { publish(panel, false, matrixStatusName(scaled.status)); return; }
            publish(panel, true, formatVector(scaled.value), "Type: real vector");
            return;
        }
        const auto b = parseAdvancedNumbers(right->text().toStdString(), 128);
        if (!b.valid()) { publish(panel, false, "B: " + errorText(b.error)); return; }
        const auto second = makeVector(b.values);
        if (!second.successful()) { publish(panel, false, matrixStatusName(second.status)); return; }
        if (op == 2) {
            const auto dot = dotVectors(first.value, second.value);
            if (!dot.successful()) { publish(panel, false, matrixStatusName(dot.status)); return; }
            publish(panel, true, number(dot.value), "Type: real scalar · dot product");
        } else {
            const auto value = op == 0 ? addVectors(first.value, second.value)
                                       : subtractVectors(first.value, second.value);
            if (!value.successful()) { publish(panel, false, matrixStatusName(value.status)); return; }
            publish(panel, true, formatVector(value.value), "Type: real vector");
        }
    });
}

void AdvancedMathPage::setupStatistics() {
    auto panel = createPanel("Statistics", "Enter a comma-separated real dataset. "
                          "Sample variance requires two or more values; percentiles use type-7 interpolation.");
    auto* data = addInput(panel, "Dataset", "2,4,4,4,5,5,7,9", "advancedStatisticsData");
    auto* mode = addChoice(panel, "Operation", {"Summary", "Percentile"}, "advancedStatisticsOperation");
    auto* pct = addInput(panel, "Percentile (0–100)", "75", "advancedStatisticsPercentile");
    auto* layout = static_cast<QVBoxLayout*>(panel.page->layout());
    panel.run->setObjectName("advancedStatisticsRun");
    layout->addWidget(panel.run);
    layout->addWidget(panel.output, 1);
    connect(panel.run, &QPushButton::clicked, this, [this, panel, data, mode, pct]() mutable {
        const auto values = parseAdvancedNumbers(data->text().toStdString(), 5000);
        if (!values.valid()) { publish(panel, false, errorText(values.error)); return; }
        if (mode->currentIndex() == 1) {
            const auto percentile = finiteNumber(pct);
            if (!percentile) { publish(panel, false, "Percentile must be finite"); return; }
            const auto result = calculatePercentile(values.values, *percentile);
            if (!result.successful()) { publish(panel, false, statisticsStatusName(result.status)); return; }
            publish(panel, true, number(result.value), "Type: real scalar · sorted type-7 percentile");
            return;
        }
        const auto result = summarizeStatistics(values.values);
        if (!result.successful()) { publish(panel, false, statisticsStatusName(result.status)); return; }
        const AdvancedMathValue typed = result;
        const auto& s = std::get<DescriptiveStatisticsResult>(typed);
        publish(panel, true, "Statistical summary",
                QString("Count: %1\nSum: %2\nMean: %3\nMedian: %4\nMin: %5\nMax: %6\nRange: %7\nPopulation variance: %8\nPopulation std dev: %9\nSample variance: %10\nSample std dev: %11")
                    .arg(static_cast<qulonglong>(s.count)).arg(number(s.sum)).arg(number(s.mean))
                    .arg(number(s.median)).arg(number(s.minimum)).arg(number(s.maximum))
                    .arg(number(s.range)).arg(number(s.populationVariance))
                    .arg(number(s.populationStdDev)).arg(number(s.sampleVariance))
                    .arg(number(s.sampleStdDev)));
    });
}

void AdvancedMathPage::setupRegression() {
    auto panel = createPanel("Regression", "Fit a bounded least-squares polynomial using matching comma-separated X/Y lists. "
                          "Coefficients use centered/scaled t, NOT raw powers of x; predictions are not confidence intervals.");
    auto* kind = addChoice(panel, "Polynomial degree", {"Linear (1)", "Quadratic (2)", "Cubic (3)", "Quartic (4)"},
                           "advancedRegressionDegree");
    auto* xs = addInput(panel, "X values", "1,2,3,4,5", "advancedRegressionX");
    auto* ys = addInput(panel, "Y values", "3,5,7,9,11", "advancedRegressionY");
    auto* xpred = addInput(panel, "Predict at X", "6", "advancedRegressionPredictX");
    auto* layout = static_cast<QVBoxLayout*>(panel.page->layout());
    panel.run->setObjectName("advancedRegressionRun");
    layout->addWidget(panel.run);
    layout->addWidget(panel.output, 1);
    connect(panel.run, &QPushButton::clicked, this, [this, panel, kind, xs, ys, xpred]() mutable {
        const auto x = parseAdvancedNumbers(xs->text().toStdString(), 5000);
        const auto y = parseAdvancedNumbers(ys->text().toStdString(), 5000);
        if (!x.valid() || !y.valid()) {
            publish(panel, false, !x.valid() ? "X: " + errorText(x.error) : "Y: " + errorText(y.error));
            return;
        }
        const auto query = finiteNumber(xpred);
        if (!query) { publish(panel, false, "Prediction X must be finite"); return; }
        const unsigned degree = static_cast<unsigned>(kind->currentIndex() + 1);
        const auto model = fitPolynomialRegression(x.values, y.values, degree);
        if (!model.successful()) { publish(panel, false, regressionStatusName(model.status)); return; }
        const auto prediction = predictRegression(model, *query);
        if (!prediction.successful()) { publish(panel, false, regressionStatusName(prediction.status)); return; }
        const AdvancedMathValue typed = model;
        const auto& fit = std::get<RegressionResult>(typed);
        QStringList coeff;
        for (const double c : fit.coefficients) coeff.append(number(c));
        QString details = QString("Degree: %1\nObservations: %2\nR²: %3\nRMSE: %4\nSSE: %5\nCentered X offset: %6\nX scale: %7\nCoefficients (powers of t): [%8]\nAdjusted R²: %9\nQR diagonal ratio: %10")
            .arg(degree).arg(static_cast<qulonglong>(fit.observations))
            .arg(number(fit.rSquared)).arg(number(fit.rmse)).arg(number(fit.sse))
            .arg(number(fit.xOffset)).arg(number(fit.xScale)).arg(coeff.join(", "))
            .arg(number(fit.adjustedRSquared)).arg(number(fit.diagonalRatio));
        if (degree == 1) {
            const auto linear = linearRegressionCoefficients(fit);
            if (linear.successful())
                details += "\nSlope: " + number(linear.slope) + "\nIntercept: " + number(linear.intercept);
            details += "\nPearson r: " + number(fit.pearsonR);
        }
        publish(panel, true, "Prediction at X=" + number(*query) + ": " + number(prediction.value), details);
    });
}

} // namespace rationalstudio
