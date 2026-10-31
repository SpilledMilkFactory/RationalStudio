#include "AdvancedMathPage.hpp"
#include "MainWindow.hpp"

#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QWidget>
#include <cmath>
#include <iostream>

using namespace rationalstudio;
namespace {
int assertions=0, failures=0;
void check(bool ok, const char* label) {
    ++assertions;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
void click(QWidget& parent, const char* name) {
    auto* run = parent.findChild<QPushButton*>(name);
    check(run != nullptr, "calculation button discoverable");
    if (run) run->click();
    QApplication::processEvents();
}
QString result(QWidget& parent, const char* name) {
    auto* output = parent.findChild<QPlainTextEdit*>(name);
    check(output != nullptr, "result field discoverable");
    return output ? output->toPlainText() : QString();
}
void set(QWidget& parent, const char* name, const QString& value) {
    auto* field = parent.findChild<QLineEdit*>(name);
    check(field != nullptr, "input field discoverable");
    if (field) field->setText(value);
}
void choose(QWidget& parent, const char* name, int index) {
    auto* mode = parent.findChild<QComboBox*>(name);
    check(mode != nullptr && mode->count() > index, "operation choice discoverable");
    if (mode && mode->count() > index) mode->setCurrentIndex(index);
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MainWindow window;
    window.resize(1150, 790);
    window.show();
    QApplication::processEvents();
    auto* tabs = window.findChild<QTabWidget*>();
    check(tabs != nullptr && tabs->count() == 4, "existing three tabs plus advanced math");
    check(tabs && tabs->tabText(0) == "Scientific" && tabs->tabText(1) == "Graphing" &&
          tabs->tabText(2) == "Exact Fractions" && tabs->tabText(3) == "Advanced Math",
          "existing tabs preserved, advanced math available");
    auto* advanced = window.findChild<AdvancedMathPage*>("advancedMathPage");
    check(advanced != nullptr, "advanced page attached to MainWindow");
    if (!advanced) return 1;
    if (tabs) tabs->setCurrentIndex(3);
    QApplication::processEvents();
    auto* modes = advanced->findChild<QTabWidget*>("advancedMathTabs");
    check(modes && modes->count() == 6, "six separate advanced workspaces");

    click(*advanced, "advancedComplexRun");
    check(result(*advanced, "advancedComplexOutput").contains("i"), "complex sqrt(-1) result");
    set(*advanced,"advancedComplexExpression", "badname");
    click(*advanced,"advancedComplexRun");
    check(result(*advanced, "advancedComplexOutput").startsWith("Error:"), "invalid complex input rejected");

    choose(*advanced, "advancedCalculusOperation", 0);
    click(*advanced, "advancedCalculusRun");
    check(result(*advanced, "advancedCalculusOutput").contains("4"), "derivative of x squared at 2");
    choose(*advanced, "advancedCalculusOperation", 1);
    click(*advanced, "advancedCalculusRun");
    check(result(*advanced, "advancedCalculusOutput").contains("9"), "integral x squared 0..3");
    choose(*advanced, "advancedCalculusOperation", 2);
    set(*advanced, "advancedCalculusExpression", "x^2-2");
    set(*advanced, "advancedCalculusB", "2");
    click(*advanced, "advancedCalculusRun");
    check(result(*advanced, "advancedCalculusOutput").contains("1.414"), "bounded root result");
    set(*advanced, "advancedCalculusA", "invalid");
    click(*advanced, "advancedCalculusRun");
    check(result(*advanced, "advancedCalculusOutput").startsWith("Error:"), "invalid numerical bound rejected");

    choose(*advanced, "advancedMatrixOperation", 0);
    click(*advanced, "advancedMatrixRun");
    check(result(*advanced, "advancedMatricesOutput").contains("[1, 3]"), "matrix transpose");
    choose(*advanced, "advancedMatrixOperation", 1);
    click(*advanced, "advancedMatrixRun");
    check(result(*advanced, "advancedMatricesOutput").contains("-2"), "matrix determinant");
    choose(*advanced, "advancedMatrixOperation", 6);
    set(*advanced, "advancedMatrixA", "3,1;1,2");
    set(*advanced, "advancedMatrixB", "9,8");
    click(*advanced, "advancedMatrixRun");
    check(result(*advanced, "advancedMatricesOutput").contains("[2, 3]"), "matrix linear system");

    choose(*advanced, "advancedVectorOperation", 2);
    click(*advanced, "advancedVectorRun");
    check(result(*advanced, "advancedVectorsOutput").contains("32"), "vector dot product");
    choose(*advanced, "advancedVectorOperation", 0);
    click(*advanced, "advancedVectorRun");
    check(result(*advanced, "advancedVectorsOutput").contains("[5, 7, 9]"), "vector addition");
    choose(*advanced, "advancedVectorOperation", 5);
    set(*advanced, "advancedVectorA", "1,2;3,4");
    set(*advanced, "advancedVectorB", "5,6");
    click(*advanced, "advancedVectorRun");
    check(result(*advanced, "advancedVectorsOutput").contains("[17, 39]"), "matrix-vector product");

    click(*advanced, "advancedStatisticsRun");
    check(result(*advanced, "advancedStatisticsOutput").contains("Mean: 5"), "descriptive summary");
    choose(*advanced, "advancedStatisticsOperation", 1);
    click(*advanced, "advancedStatisticsRun");
    check(result(*advanced, "advancedStatisticsOutput").contains("5.5"), "type7 percentile");
    click(*advanced, "advancedRegressionRun");
    const auto regression = result(*advanced, "advancedRegressionOutput");
    check(regression.contains("13") && regression.contains("Slope: 2"), "linear fit and prediction");
    set(*advanced, "advancedRegressionY", "1,2");
    click(*advanced, "advancedRegressionRun");
    check(result(*advanced, "advancedRegressionOutput").startsWith("Error:"), "mismatched observations rejected");

    // Ordinary Scientific and Graphing widgets survive the new tab.
    check(window.findChild<QLineEdit*>("ScientificExpression") != nullptr,
          "ordinary scientific expression remains available");
    check(window.findChild<QPushButton*>("graphFindIntersections") != nullptr,
          "graphing analysis controls remain available");
    std::cout << (failures ? "FAIL: " : "PASS: ") << assertions
              << " AdvancedMathGui assertions\n";
    return failures ? 1 : 0;
}
