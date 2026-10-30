#include "MainWindow.hpp"
#include "GraphCanvas.hpp"
#include "GraphWorkspacePersistence.hpp"
#include "MathExpressionEdit.hpp"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QFile>
#include <QLabel>
#include <QList>
#include <QStringList>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QToolButton>
#include <QWidget>

#include <algorithm>
#include <iostream>

using namespace rationalstudio;
namespace {
int assertions = 0;
int failures = 0;
void check(bool pass, const char* message) {
    ++assertions;
    if (!pass) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
int editors(MainWindow& window, const QString& prefix) {
    int found = 0;
    for (auto* input : window.findChildren<MathExpressionEdit*>())
        if (input->objectName().startsWith(prefix)) ++found;
    return found;
}
MathExpressionEdit* findEditor(MainWindow& window, const QString& prefix) {
    for (auto* input : window.findChildren<MathExpressionEdit*>())
        if (input->objectName().startsWith(prefix)) return input;
    return nullptr;
}
GraphCanvas* findCanvas(MainWindow& window) {
    for (auto* widget : window.findChildren<QWidget*>())
        if (auto* canvas = dynamic_cast<GraphCanvas*>(widget)) return canvas;
    return nullptr;
}
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MainWindow window;
    window.resize(1120, 760);
    window.show();
    if (auto* tabs = window.findChild<QTabWidget*>()) tabs->setCurrentIndex(1);
    QApplication::processEvents();

    // Existing Qt tests locate these stable button names. The two-column
    // layout must retain them and not clip buttons at the sidebar minimum.
    const QStringList names = {"addCartesianGraphRow", "addParametricGraphRow",
                               "addPolarGraphRow", "addImplicitGraphRow",
                               "addInequalityGraphRow"};
    QList<QPushButton*> buttons;
    for (const QString& name : names) {
        auto* button = window.findChild<QPushButton*>(name);
        check(button != nullptr, "every graph representation has a stable add control");
        if (button) {
            buttons.push_back(button);
            check(button->isVisible() && !button->toolTip().isEmpty(),
                  "graph add controls visible with instructions");
            check(button->width() > 65 && button->width() < 240,
                  "graph add controls remain usable without a five-button horizontal row");
            check(!button->accessibleName().isEmpty(),
                  "graph add controls have accessible names");
        }
    }
    auto* guide = window.findChild<QToolButton*>("graphRepresentationHelpButton");
    auto* guideText = window.findChild<QLabel*>("graphRepresentationHelpText");
    check(guide != nullptr && guideText != nullptr, "type guide controls discoverable");
    if (guide && guideText) {
        check(!guide->isChecked() && !guideText->isVisible(),
              "syntax guide is collapsed initially");
        guide->click();
        QApplication::processEvents();
        check(guideText->isVisible() && guideText->text().contains("Implicit") &&
              guideText->text().contains("Inequality"), "syntax guide explains supported representations");
        guide->click();
        QApplication::processEvents();
        check(!guideText->isVisible(), "syntax guide collapses without destroying controls");
    }

    // The page starts with two Cartesian rows. Each button must create only
    // its own representation and focus the new editor for immediate typing.
    if (buttons.size() == names.size()) {
        const int initial = editors(window, "GraphExpression_");
        buttons[0]->click();
        check(editors(window, "GraphExpression_") == initial + 1,
              "Cartesian add creates a distinct explicit row");
        auto* cart = findEditor(window, "GraphExpression_");
        check(cart != nullptr, "existing Cartesian editors remain reachable");
        // New row is the most recent editor of this kind.
        MathExpressionEdit* newCartesian = nullptr;
        for (auto* input : window.findChildren<MathExpressionEdit*>())
            if (input->objectName().startsWith("GraphExpression_") && input->text().isEmpty())
                newCartesian = input;
        check(newCartesian && newCartesian->hasFocus(),
              "new Cartesian row receives keyboard focus");
        if (newCartesian) newCartesian->setText("y = x + 7");

        buttons[1]->click();
        check(editors(window, "ParametricX_") == 1 &&
              editors(window, "ParametricY_") == 1,
              "Parametric add provides both coordinate editors");
        check(findEditor(window, "ParametricX_") &&
              findEditor(window, "ParametricX_")->hasFocus(),
              "Parametric row focuses first coordinate editor");
        buttons[2]->click();
        check(editors(window, "PolarRadius_") == 1,
              "Polar add provides radius editor");
        buttons[3]->click();
        check(editors(window, "ImplicitEquation_") == 1,
              "Implicit add provides equation editor");
        buttons[4]->click();
        check(editors(window, "InequalityEquation_") == 1,
              "Inequality add provides region editor");
    }
    check(window.findChildren<QLabel*>("graphRepresentationBadge").size() >= 5,
          "explicit, implicit, and inequality rows have readable type badges");
    check(window.findChildren<QDoubleSpinBox*>("parametricMinimum").size() == 1 &&
          window.findChildren<QDoubleSpinBox*>("parametricMaximum").size() == 1,
          "Parametric interval editor remains available");
    check(window.findChildren<QDoubleSpinBox*>("polarMinimum").size() == 1 &&
          window.findChildren<QDoubleSpinBox*>("polarMaximum").size() == 1,
          "Polar interval editor remains available");
    check(findCanvas(window) != nullptr, "shared graph renderer remains present");
    QTemporaryDir temp;
    check(temp.isValid(), "temporary workspace available");
    if (temp.isValid()) {
        QString error;
        const auto filename = temp.filePath("mixed-issue43.json");
        check(window.saveWorkspace(filename, &error), "mixed representation workspace saves");
        QFile file(filename);
        check(file.open(QIODevice::ReadOnly), "saved workspace readable");
        if (file.isOpen()) {
            GraphWorkspaceState state;
            check(GraphWorkspacePersistence::decode(file.readAll(), state, &error),
                  "existing version-3 decoder accepts mixed representation controls");
            check(state.curves.size() == 7, "all seven graph rows persisted");
            int parametric = 0, polar = 0, implicit = 0, inequality = 0;
            for (const auto& row : state.curves) {
                parametric += row.parametric;
                polar += row.polar;
                implicit += row.implicit;
                inequality += row.inequality;
            }
            check(parametric == 1 && polar == 1 && implicit == 1 && inequality == 1,
                  "mixed row types survive existing serialization");
        }
        check(window.loadWorkspace(filename, &error), "mixed representation workspace reloads");
        check(editors(window, "ParametricX_") == 1 &&
              editors(window, "PolarRadius_") == 1 &&
              editors(window, "ImplicitEquation_") == 1 &&
              editors(window, "InequalityEquation_") == 1,
              "mixed graph editors restored without UI representation drift");
    }
    std::cout << (failures ? "FAIL: " : "PASS: ") << assertions
              << " GraphRepresentationControls assertions\n";
    return failures ? 1 : 0;
}
