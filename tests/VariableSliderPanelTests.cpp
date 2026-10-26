#include "VariableSliderPanel.hpp"
#include "MainWindow.hpp"
#include <QTabWidget>

#include <QApplication>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

using namespace rationalstudio;
namespace {
int checks = 0;
int fails = 0;
void check(bool ok, const std::string& msg) {
    ++checks;
    if (!ok) { ++fails; std::cerr << "FAIL: " << msg << '\n'; }
}
void near(double actual, double target, const std::string& label) {
    check(std::isfinite(actual) && std::abs(actual - target) < 1e-8, label);
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    VariableSliderPanel panel;
    panel.resize(390, 500);
    panel.show();
    app.processEvents();

    std::string error;
    check(panel.rowCount() == 0, "empty initially");
    check(panel.addVariable("a", 2, -2, 2, .5, &error), "add a");
    check(error.empty() && panel.rowCount() == 1, "row created");
    check(!panel.addVariable("A", 1, 0, 10, 1, &error), "case duplicate");
    check(!error.empty(), "duplicate feedback");
    check(!panel.addVariable("sin", 1, 0, 10, 1), "reserved name");
    check(!panel.addVariable("2bad", 1, 0, 10, 1), "malformed name");
    check(!panel.addVariable("b", 0, 1, 1, 1), "invalid range");
    check(!panel.addVariable("b", 0, 0, 1, .00000000001), "unrepresentable step");
    check(!panel.addVariable("b", 7, 0, 5, 1), "out of range initial");
    near(panel.value("a").value_or(-99), 2, "initial assigned");

    auto* slider = panel.findChild<QSlider*>("variableSlider_a");
    auto* spin = panel.findChild<QDoubleSpinBox*>("variableValue_a");
    auto* minSpin = panel.findChild<QDoubleSpinBox*>("variableMin_a");
    auto* maxSpin = panel.findChild<QDoubleSpinBox*>("variableMax_a");
    auto* stepSpin = panel.findChild<QDoubleSpinBox*>("variableStep_a");
    check(slider && spin && minSpin && maxSpin && stepSpin, "Qt controls created");
    if (!slider || !spin || !minSpin || !maxSpin || !stepSpin) return 1;
    check(slider->maximum() == 8 && slider->value() == 8, "tick mapped");

    int calls = 0;
    double last = -999;
    panel.onValueChanged = [&](const std::string& name, double v) {
        check(name == "a", "callback name");
        ++calls;
        last = v;
    };
    slider->setValue(3);
    near(panel.value("a").value_or(-999), -.5, "slider sets core");
    near(spin->value(), -.5, "slider synchronizes spin");
    check(calls == 1 && last == -.5, "single callback");

    spin->setValue(1.24); // snaps to 1.0 on 0.5 grid
    near(panel.value("a").value_or(-999), 1, "spin snaps");
    near(spin->value(), 1, "spin reflects snapped value");
    check(slider->value() == 6, "spin synchronizes slider");

    check(panel.definitions().assign("b = a + 3").accepted,
          "dependent definition may be introduced by core owner");
    check(panel.setVariableValue("a", 2), "set value programmatically");
    near(panel.definitions().value("b").value_or(-1), 5, "dependent recomputes");
    check(!panel.removeVariable("a", &error), "cannot remove depended upon var");
    check(panel.rowCount() == 1, "rejected remove preserves slider");

    check(panel.configureVariable("a", -5, 5, 2, &error), "change range");
    check(error.empty(), "valid range clears error");
    check(slider->maximum() == 5, "new tick count");
    near(panel.value("a").value_or(-1), 3, "reconfigure snaps current");
    near(panel.definitions().value("b").value_or(-1), 6, "reconfigure propagates");
    check(!panel.configureVariable("a", 9, 2, 1, &error), "invalid range rejected");
    check(slider->maximum() == 5, "invalid config preserves scale");
    check(!panel.configureVariable("a", -1, 1, .000001), "too many ticks rejected");
    near(panel.value("a").value_or(-1), 3, "invalid config preserves binding");
    check(!panel.setVariableValue("missing", 1), "unknown value rejected");
    check(!panel.setVariableValue("a", std::numeric_limits<double>::infinity()), "infinite value rejected");

    // GUI range edits fire immediately and stay synchronized.
    minSpin->setValue(-3);
    near(panel.value("a").value_or(-1), 3, "GUI min update");
    check(maxSpin->value() == 5, "GUI range controls stable");
    stepSpin->setValue(1);
    check(slider->maximum() == 8, "GUI step update");

    check(panel.definitions().erase("b").accepted, "remove dependent definition");
    check(panel.removeVariable("a", &error), "remove unreferenced var");
    check(panel.rowCount() == 0 && !panel.value("a"), "removal updates store");

    // Exercise real add button and keyboard input.
    auto* nameInput = panel.findChild<QLineEdit*>("newVariableName");
    auto* button = panel.findChild<QPushButton*>("addVariableSlider");
    check(nameInput && button, "add UI found");
    if (nameInput && button) {
        nameInput->setText("offset");
        button->click();
        check(panel.rowCount() == 1 && panel.value("offset").has_value(), "add via button");
    }
    app.processEvents();

    // Issue #30: Verify the actual application exposes slider controls.
    {
        MainWindow window;
        window.resize(1250, 790);
        window.show();  

        auto* tabs = window.findChild<QTabWidget*>();   

        check(
            tabs != nullptr && tabs->count() >= 2,
            "MainWindow graphing tab exists"
        );  

        if (tabs) {
            tabs->setCurrentIndex(1);
        }   

        app.processEvents();    

        auto* embeddedPanel =
            window.findChild<QWidget*>("variableSliderPanel");  

        check(
            embeddedPanel != nullptr,
            "MainWindow embeds VariableSliderPanel"
        );  

        check(
            embeddedPanel &&
                embeddedPanel->isVisibleTo(&window),
            "Slider panel visible in graphing workspace"
        );  

        check(
            embeddedPanel &&
                embeddedPanel->findChild<QPushButton*>(
                    "addVariableSlider"
                ) != nullptr,
            "MainWindow exposes the Add Slider button"
        );
    }
    std::cout << (fails ? "FAIL: " : "PASS: ") << checks
              << " VariableSliderPanel assertions\n";
    return fails ? 1 : 0;
}
