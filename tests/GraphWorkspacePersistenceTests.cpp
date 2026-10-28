#include "GraphWorkspacePersistence.hpp"
#include "MainWindow.hpp"
#include "MathExpressionEdit.hpp"
#include "VariableSliderPanel.hpp"

#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QWidget>

#include <cmath>
#include <iostream>

using namespace rationalstudio;
namespace {
int checks = 0, failures = 0;
void check(bool value, const char* description) {
    ++checks;
    if (!value) { ++failures; std::cerr << "FAIL: " << description << '\n'; }
}
VariableSliderPanel* panel(MainWindow& window) {
    for (QWidget* item : window.findChildren<QWidget*>()) {
        if (auto* value = dynamic_cast<VariableSliderPanel*>(item)) return value;
    }
    return nullptr;
}
MathExpressionEdit* graphEditor(MainWindow& window) {
    for (auto* item : window.findChildren<MathExpressionEdit*>()) {
        if (item->objectName().startsWith("GraphExpression_")) return item;
    }
    return nullptr;
}
QByteArray docWith(const std::function<void(QJsonObject&)>& edit, const QByteArray& bytes) {
    auto root = QJsonDocument::fromJson(bytes).object();
    edit(root);
    return QJsonDocument(root).toJson();
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    GraphWorkspaceState state;
    state.centerX = 2.25;
    state.centerY = -1.5;
    state.pixelsPerUnit = 81;
    state.angle = AngleMode::Degrees;
    state.curves.push_back({"y = f(x)", true, QColor("#50c4ef")});
    state.curves.push_back({"y = sin(x)", false, QColor("#f07882")});
    state.sliders.push_back({"a", 2.5, -5, 5, 0.5});
    state.assignments.push_back({"b", "a+3"});
    state.functions.push_back({"f", "b*x", {"x"}});
    state.functions.push_back({"g", "f(x)+2", {"x"}});

    QString error;
    GraphWorkspaceState decoded;
    const auto raw = GraphWorkspacePersistence::encode(state);
    check(GraphWorkspacePersistence::decode(raw, decoded, &error), "decode valid v2 workspace");
    check(decoded.curves.size() == 2, "curve count roundtrip");
    check(decoded.curves[0].expression == "y = f(x)", "curve source preserved");
    check(!decoded.curves[1].visible, "visibility preserved");
    check(decoded.curves[1].color == QColor("#f07882"), "color preserved");
    check(decoded.centerX == 2.25 && decoded.centerY == -1.5, "viewport center preserved");
    check(decoded.pixelsPerUnit == 81, "viewport scale preserved");
    check(decoded.angle == AngleMode::Degrees, "angle mode preserved");
    check(decoded.sliders.size() == 1 && decoded.sliders[0].value == 2.5,
          "slider current value preserved");
    check(decoded.sliders[0].minimum == -5 && decoded.sliders[0].maximum == 5 &&
          decoded.sliders[0].step == 0.5, "slider range/step preserved");
    check(decoded.validatedVariables.value("b").value_or(-1) == 5.5,
          "scalar assignment dependency resolves after reload");
    check(decoded.validatedFunctions.contains("f") && decoded.validatedFunctions.contains("g"),
          "functions restored");
    Expression expr("g(2)", decoded.validatedVariables.scope(), decoded.validatedFunctions.scope());
    check(expr.valid(), "restored nested function compiles");
    if (expr.valid()) check(std::abs(decoded.validatedVariables.evaluate(expr) - 13.) < 1e-10,
                             "restored transitive variable/function evaluation");
    check(GraphWorkspacePersistence::orderedAssignments(decoded.validatedVariables,
            decoded.sliders).size() == 1, "assignment snapshot excludes sliders");
    check(GraphWorkspacePersistence::orderedFunctions(decoded.validatedFunctions).size() == 2,
          "function snapshot includes dependencies");

    const auto reject = [&](const char* description, const std::function<void(QJsonObject&)>& edit) {
        GraphWorkspaceState untouched;
        check(!GraphWorkspacePersistence::decode(docWith(edit, raw), untouched, &error), description);
    };
    reject("missing slider array rejected", [](QJsonObject& root) { root.remove("sliders"); });
    reject("malformed slider type rejected", [](QJsonObject& root) {
        auto a = root.value("sliders").toArray();
        auto s = a[0].toObject(); s["step"] = "bad"; a[0] = s; root["sliders"] = a;
    });
    reject("zero slider step rejected", [](QJsonObject& root) {
        auto a = root.value("sliders").toArray();
        auto s = a[0].toObject(); s["step"] = 0; a[0] = s; root["sliders"] = a;
    });
    reject("duplicate slider rejected", [](QJsonObject& root) {
        auto a = root.value("sliders").toArray(); a.append(a[0]); root["sliders"] = a;
    });
    reject("unresolved assignment rejected", [](QJsonObject& root) {
        auto a = root.value("variables").toArray();
        auto s = a[0].toObject(); s["expression"] = "missing+1"; a[0] = s; root["variables"] = a;
    });
    reject("unresolved function rejected", [](QJsonObject& root) {
        auto a = root.value("functions").toArray();
        auto s = a[0].toObject(); s["body"] = "missing(x)"; a[0] = s; root["functions"] = a;
    });
    reject("invalid viewport rejected", [](QJsonObject& root) { root["pixelsPerUnit"] = -1; });
    reject("non-string curve rejected", [](QJsonObject& root) {
        auto a = root.value("expressions").toArray();
        auto s = a[0].toObject(); s["expression"] = 7; a[0] = s; root["expressions"] = a;
    });
    reject("unknown version rejected", [](QJsonObject& root) { root["version"] = 99; });
    check(!GraphWorkspacePersistence::decode("not-json", decoded, &error), "malformed JSON rejected");

    // Existing version-1 graph files remain loadable with empty definitions.
    QJsonObject legacy;
    legacy["format"] = "RationalStudioGraph";
    legacy["version"] = 1;
    legacy["expressions"] = QJsonArray{QJsonObject{{"expression", "y = x^2"},
                                                   {"visible", true}, {"color", "#ff0000"}}};
    GraphWorkspaceState old;
    check(GraphWorkspacePersistence::decode(QJsonDocument(legacy).toJson(), old, &error),
          "legacy version 1 decodes");
    check(old.sliders.empty() && old.assignments.empty() && old.functions.empty(),
          "legacy has no invented definitions");

    QTemporaryDir temp;
    check(temp.isValid(), "temporary directory created");
    const QString filePath = temp.filePath("workspace.json");
    QFile data(filePath);
    check(data.open(QIODevice::WriteOnly), "fixture workspace opened for writing");
    check(data.write(raw) == raw.size(), "fixture workspace written");
    data.close();
    MainWindow window;
    window.resize(1200, 800);
    window.show();
    if (auto* tabs = window.findChild<QTabWidget*>()) tabs->setCurrentIndex(1);
    app.processEvents();
    check(window.loadWorkspace(filePath, &error), "real MainWindow loads version 2 workspace");
    auto* variables = panel(window);
    check(variables != nullptr, "real slider panel located");
    if (variables) {
        check(variables->rowCount() == 1, "restored slider row visible");
        check(std::abs(variables->value("a").value_or(-1) - 2.5) < 1e-12,
              "restored slider numeric value");
        check(std::abs(variables->value("b").value_or(-1) - 5.5) < 1e-12,
              "restored dependent scalar value");
    }
    auto* editor = graphEditor(window);
    check(editor && editor->text() == "y = f(x)", "graph source restored");
    if (editor) check(editor->property("syntaxState").toString() == "valid",
                       "restored function expression compiles in real MainWindow");
    const QString outputPath = temp.filePath("saved.json");
    check(window.saveWorkspace(outputPath, &error), "real MainWindow saves version 2 workspace");
    QFile saved(outputPath);
    check(saved.open(QIODevice::ReadOnly), "saved file readable");
    GraphWorkspaceState copy;
    check(GraphWorkspacePersistence::decode(saved.readAll(), copy, &error),
          "real saved workspace parses and validates");
    check(copy.functions.size() == 2 && copy.assignments.size() == 1,
          "roundtrip retained functions and computed assignments");
    check(copy.sliders.size() == 1 && copy.sliders[0].step == 0.5,
          "roundtrip retained slider precision");

    QFile invalid(temp.filePath("bad.json"));
    check(invalid.open(QIODevice::WriteOnly), "invalid file opened");
    invalid.write(docWith([](QJsonObject& root) { root["version"] = 31; }, raw));
    invalid.close();
    check(!window.loadWorkspace(invalid.fileName(), &error), "bad workspace rejected in MainWindow");
    check(variables && variables->value("b").value_or(-1) == 5.5,
          "rejected load retains previous scalar definitions");
    check(editor && editor->text() == "y = f(x)",
          "rejected load retains graph expression");
    if (variables) {
        check(!variables->removeVariable("a"),
              "cannot remove variable captured by restored function");
        check(variables->setVariableValue("a", 3), "restored slider remains interactive");
        check(variables->value("b").value_or(-1) == 6, "dependent scalar updates after slider move");
    }
    check(window.loadWorkspace(temp.filePath("workspace.json"), &error),
          "reload original workspace after edits");
    QFile legacyFile(temp.filePath("old.json"));
    check(legacyFile.open(QIODevice::WriteOnly), "legacy file opened");
    legacyFile.write(QJsonDocument(legacy).toJson()); legacyFile.close();
    check(window.loadWorkspace(legacyFile.fileName(), &error), "legacy load into populated session");
    auto* legacyPanel = panel(window);
    check(legacyPanel && legacyPanel->rowCount() == 0, "legacy load clears prior session sliders");
    auto* legacyEditor = graphEditor(window);
    check(legacyEditor && legacyEditor->text() == "y = x^2", "legacy curve restored");
    std::cout << (failures ? "FAIL: " : "PASS: ") << checks
              << " GraphWorkspacePersistence assertions\n";
    return failures ? 1 : 0;
}
