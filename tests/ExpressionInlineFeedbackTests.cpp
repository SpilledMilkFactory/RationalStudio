#include "MainWindow.hpp"
#include "MathExpressionEdit.hpp"
#include "GraphCanvas.hpp"

#include <QApplication>
#include <QEventLoop>
#include <QLabel>
#include <QTabWidget>
#include <QTimer>
#include <iostream>

using namespace rationalstudio;
namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* description) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << description << '\n'; }
}
void flush() {
    QEventLoop loop;
    QTimer::singleShot(250, &loop, &QEventLoop::quit);
    loop.exec();
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MainWindow window;
    window.resize(1250, 790);
    window.show();
    auto* tabs = window.findChild<QTabWidget*>();
    check(tabs && tabs->count() >= 2, "graphing tab available");
    if (tabs) tabs->setCurrentIndex(1);
    app.processEvents();

    auto* scientific = window.findChild<MathExpressionEdit*>("ScientificExpression");
    check(scientific != nullptr, "scientific MathExpressionEdit integrated");
    if (scientific) {
        scientific->setText("π+*");
        check(scientific->property("syntaxState").toString() == "invalid",
              "scientific invalid state");
        check(scientific->toolTip().contains("Column 3"),
              "scientific Unicode error column");
        scientific->setText("badname");
        check(scientific->toolTip().contains("Unknown identifier"),
              "scientific unknown identifier message");
        scientific->setText("√(9)+π");
        check(scientific->property("syntaxState").toString() == "valid",
              "scientific valid recovery");
        check(scientific->toolTip().isEmpty(), "scientific error tooltip removed on recovery");
    }

    auto editors = window.findChildren<MathExpressionEdit*>();
    MathExpressionEdit* first = nullptr;
    MathExpressionEdit* second = nullptr;
    for (auto* e : editors) {
        if (!e->objectName().startsWith("GraphExpression_")) continue;
        if (!first) first = e;
        else if (!second) second = e;
    }
    check(first && second, "two graph expression editors available");
    // GraphCanvas intentionally has no Q_OBJECT; do not use typed findChild.
    GraphCanvas* graph = nullptr;
    for (auto* widget : window.findChildren<QWidget*>()) {
        if (auto* candidate = dynamic_cast<GraphCanvas*>(widget)) {
            graph = candidate;
            break;
        }
    }
    check(graph != nullptr, "actual GraphCanvas available");
    if (first && second) {
        first->setText("y = π+*");
        check(first->property("syntaxState").toString() == "invalid",
              "graph malformed operator classified invalid");
        check(first->toolTip().contains("Column 3"),
              "graph Unicode column maps to displayed body");
        second->setText("y = sin(x)");
        check(second->property("syntaxState").toString() == "valid",
              "unrelated graph row stays valid during malformed edit");
        flush(); // allow graph debounce and subsequent canvas paint
        check(second->property("syntaxState").toString() == "valid",
              "unrelated graph row valid after rebuild");
        if (graph) {
            graph->grab();
            check(graph->isVisible(), "graph canvas remains present while a row is invalid");
        }
        first->setText("y = oops");
        check(first->toolTip().contains("Unknown identifier") &&
              first->toolTip().contains("slider"),
              "unknown graph name receives actionable diagnostic");
        first->setText("y = sqrt(");
        check(first->property("syntaxState").toString() == "incomplete",
              "unfinished function stays incomplete");
        first->setText("y = √(x²)");
        check(first->property("syntaxState").toString() == "valid",
              "valid Unicode graph expression recovers");
        check(first->toolTip().isEmpty(), "graph error tooltip cleared on recovery");
        flush();
        check(second->property("syntaxState").toString() == "valid",
              "other row remains valid after repair");
    }
    std::cout << (failures ? "FAIL: " : "PASS: ") << checks
              << " ExpressionInlineFeedback assertions\n";
    return failures ? 1 : 0;
}
