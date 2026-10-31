#include "MainWindow.hpp"
#include "AdvancedMathPage.hpp"
#include "GraphCanvas.hpp"
#include "MathExpressionEdit.hpp"
#include "VariableSliderPanel.hpp"

#include <QAccessible>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QToolButton>
#include <QWidget>

#include <iostream>

using namespace rationalstudio;
namespace {
int checks = 0;
int failures = 0;
void check(bool condition, const char* message) {
    ++checks;
    if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
// Qt 6's QObject::findChild<Derived*>() requires Derived to declare
// Q_OBJECT. These existing QWidget subclasses do not need a custom
// meta-object, so discover them through QWidget and use checked RTTI.
template <typename Widget>
Widget* findWidget(QWidget& root) {
    for (QWidget* child : root.findChildren<QWidget*>()) {
        if (auto* widget = dynamic_cast<Widget*>(child))
            return widget;
    }
    return nullptr;
}
void exposed(QWidget* widget, const char* description) {
    check(widget != nullptr, description);
    if (!widget) return;
    check(!widget->accessibleName().trimmed().isEmpty(), "nonempty accessible name");
    auto* interface = QAccessible::queryAccessibleInterface(widget);
    check(interface != nullptr, "Qt accessible interface available");
    if (interface) {
        check(interface->role() != QAccessible::NoRole, "nonempty Qt accessible role");
        check(!interface->text(QAccessible::Name).trimmed().isEmpty(),
              "accessible name exposed through Qt accessibility interface");
    }
}
} // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QTemporaryDir isolated;
    check(isolated.isValid(), "settings isolated from user's desktop");
    if (!isolated.isValid()) return 2;
    MainWindow window(nullptr, isolated.filePath("accessibility.ini"));
    window.show();
    app.processEvents();

    auto* tabs = window.findChild<QTabWidget*>();
    check(tabs && tabs->count() == 4, "four workspaces preserve navigation order");
    if (!tabs) return 2;
    exposed(tabs, "top-level calculator tabs");
    exposed(window.findChild<MathExpressionEdit*>("ScientificExpression"),
            "scientific expression editor");
    auto* graph = findWidget<GraphCanvas>(window);
    exposed(graph, "graph canvas has textual alternative");
    check(graph && graph->accessibleDescription().contains("Intersections"),
          "graph describes accessible textual alternative");

    auto* advanced = window.findChild<AdvancedMathPage*>("advancedMathPage");
    check(advanced != nullptr, "advanced mathematics workspace");
    auto* inner = window.findChild<QTabWidget*>("advancedMathTabs");
    exposed(inner, "six advanced operation tabs");
    check(inner && inner->count() == 6, "advanced six panels retained");
    for (int i = 0; inner && i < inner->count(); ++i) {
        inner->setCurrentIndex(i);
        check(!inner->tabText(i).isEmpty(), "advanced tab has visible label");
    }

    auto* complexInput = window.findChild<QLineEdit*>("advancedComplexExpression");
    exposed(complexInput, "advanced complex expression");
    check(complexInput && complexInput->accessibleName() == "Complex expression",
          "human-readable label rather than raw internal object name");
    auto* complexButton = window.findChild<QPushButton*>("advancedComplexRun");
    exposed(complexButton, "calculate complex action");
    auto* output = window.findChild<QPlainTextEdit*>("advancedComplexOutput");
    exposed(output, "complex calculation result");
    check(output && output->isReadOnly(), "accessible result is readable without editing");
    if (inner) inner->setCurrentIndex(0);
    if (complexButton && output) {
        complexButton->click();
        app.processEvents();
        check(output->toPlainText().startsWith("Result:"), "committed result displayed");
    }

    auto* sliders = findWidget<VariableSliderPanel>(window);
    check(sliders != nullptr, "slider owner present");
    if (sliders) {
        std::string error;
        check(sliders->addVariable("a", 2, -10, 10, 0.5, &error),
              "accessible dynamic variable row creates successfully");
        exposed(sliders->findChild<QDoubleSpinBox*>("variableValue_a"),
                "variable value spinbox");
        exposed(sliders->findChild<QSlider*>("variableSlider_a"),
                "variable value slider");
        exposed(sliders->findChild<QToolButton*>("removeVariable_a"),
                "remove-variable icon control");
        exposed(sliders->findChild<QDoubleSpinBox*>("variableMin_a"),
                "variable minimum field");
        exposed(sliders->findChild<QDoubleSpinBox*>("variableMax_a"),
                "variable maximum field");
        exposed(sliders->findChild<QDoubleSpinBox*>("variableStep_a"),
                "variable step field");
    }

    tabs->setCurrentIndex(1);
    app.processEvents();
    auto* graphInput = window.findChild<MathExpressionEdit*>("GraphExpression_1");
    if (!graphInput) {
        for (auto* input : window.findChildren<MathExpressionEdit*>())
            if (input->objectName().startsWith("GraphExpression_")) {
                graphInput = input; break;
            }
    }
    exposed(graphInput, "dynamic Cartesian graph editor");
    if (graphInput) {
        graphInput->setText("sin(");
        app.processEvents();
        check(!graphInput->accessibleDescription().isEmpty(),
              "syntax feedback exposed in graph input accessible description");
        graphInput->setText("y = x^2");
        app.processEvents();
        check(!graphInput->accessibleDescription().isEmpty(),
              "accessible graph validation updates after repair");
    }

    auto* scientific = window.findChild<MathExpressionEdit*>("ScientificExpression");
    if (scientific) {
        tabs->setCurrentIndex(0);
        scientific->setText("sin(");
        app.processEvents();
        check(!scientific->accessibleDescription().isEmpty(),
              "scientific parser diagnostics have accessible text");
        scientific->setText("2+3");
        app.processEvents();
        check(scientific->accessibleDescription().contains("Valid"),
              "repaired expression clears stale error description");
        scientific->setFocus(Qt::TabFocusReason);
        check(scientific->hasFocus(), "keyboard focus retained in scientific entry");
    }

    auto* light = window.findChild<QAction*>("preferenceLight");
    check(light != nullptr, "light appearance action retained");
    if (light) {
        light->trigger();
        check(window.styleSheet().contains("QToolButton:focus"),
              "visible focus styling in light appearance");
    }
    auto* dark = window.findChild<QAction*>("preferenceDark");
    check(dark != nullptr, "dark appearance action retained");
    if (dark) {
        dark->trigger();
        check(window.styleSheet().contains("QToolButton:focus"),
              "visible focus styling in dark appearance");
    }
    check(window.findChild<QAction*>("keyboardTab1") != nullptr,
          "Issue 56 keyboard shortcuts preserved");
    check(window.findChild<QAction*>("keyboardShortcutsHelp") != nullptr,
          "Issue 56 discoverable help preserved");

    std::cout << (failures ? "FAIL: " : "PASS: ") << checks
              << " AccessibilityGui assertions\n";
    return failures ? 1 : 0;
}
