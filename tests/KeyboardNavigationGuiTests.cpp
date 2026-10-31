#include "MainWindow.hpp"
#include "MathExpressionEdit.hpp"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextBrowser>

#include <iostream>
#include <set>

using namespace rationalstudio;
namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* message) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
QAction* action(MainWindow& w, const char* name) {
    return w.findChild<QAction*>(QString::fromLatin1(name));
}
void press(QWidget* w, Qt::Key key, Qt::KeyboardModifiers modifiers) {
    QKeyEvent event(QEvent::KeyPress, key, modifiers);
    QApplication::sendEvent(w, &event);
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QTemporaryDir isolated;
    check(isolated.isValid(), "temporary preferences isolation");
    if (!isolated.isValid()) return 2;
    MainWindow w(nullptr, isolated.filePath("keyboard-navigation.ini"));
    w.show();
    app.processEvents();
    const auto tabs = w.findChild<QTabWidget*>();
    check(tabs && tabs->count() == 4, "four original calculator tabs preserved");
    check(w.findChild<QMenu*>("keyboardNavigationMenu") != nullptr,
          "discoverable Navigation menu");
    check(w.findChild<QMenu*>("keyboardGraphMenu") != nullptr,
          "discoverable Graph menu");
    check(w.findChild<QMenu*>("keyboardHelpMenu") != nullptr,
          "discoverable Help menu");
    if (!tabs) return 2;

    // Check every registered shortcut is unique. Duplicate window-level
    // accelerators make behavior focus-dependent or ambiguous.
    const std::pair<const char*, const char*> keys[] = {
        {"keyboardTab1", "Ctrl+1"}, {"keyboardTab2", "Ctrl+2"},
        {"keyboardTab3", "Ctrl+3"}, {"keyboardTab4", "Ctrl+4"},
        {"keyboardNextTab", "Ctrl+Tab"}, {"keyboardPreviousTab", "Ctrl+Shift+Tab"},
        {"keyboardFocusInput", "Ctrl+L"}, {"keyboardFocusGraph", "Ctrl+Shift+L"},
        {"keyboardNextAdvanced", "Ctrl+Alt+PgDown"},
        {"keyboardPreviousAdvanced", "Ctrl+Alt+PgUp"},
        {"keyboardGraphAdd", "Ctrl+N"}, {"keyboardGraphSave", "Ctrl+S"},
        {"keyboardGraphOpen", "Ctrl+O"}, {"keyboardGraphExport", "Ctrl+Shift+E"},
        {"keyboardGraphHome", "Ctrl+0"},
        {"keyboardGraphIntersections", "Ctrl+Shift+I"},
        {"keyboardGraphFeatures", "Ctrl+Shift+F"},
        {"keyboardShortcutsHelp", "F1"},
    };
    std::set<QString> seen;
    for (const auto& [name, key] : keys) {
        auto* a = action(w, name);
        check(a != nullptr, "shortcut command exists");
        if (!a) continue;
        check(a->shortcut() == QKeySequence(QString::fromLatin1(key)),
              "documented accelerator registered");
        check(a->shortcutContext() == Qt::WindowShortcut,
              "shortcut scoped to application window");
        check(seen.insert(a->shortcut().toString(QKeySequence::PortableText)).second,
              "no duplicate window accelerator");
        check(!a->autoRepeat(), "commands do not autorepeat");
    }

    auto* input = w.findChild<MathExpressionEdit*>("ScientificExpression");
    check(input != nullptr, "scientific input still available");
    auto* tab2 = action(w, "keyboardTab2");
    auto* tab3 = action(w, "keyboardTab3");
    auto* tab4 = action(w, "keyboardTab4");
    auto* tab1 = action(w, "keyboardTab1");
    auto* next = action(w, "keyboardNextTab");
    auto* prior = action(w, "keyboardPreviousTab");
    auto* focus = action(w, "keyboardFocusInput");
    auto* graphFocus = action(w, "keyboardFocusGraph");
    auto* add = action(w, "keyboardGraphAdd");
    auto* nextAdvanced = action(w, "keyboardNextAdvanced");
    auto* priorAdvanced = action(w, "keyboardPreviousAdvanced");
    if (!input || !tab1 || !tab2 || !tab3 || !tab4 || !next || !prior || !focus ||
        !graphFocus || !add || !nextAdvanced || !priorAdvanced) return 2;

    input->setText("12+3");
    tab2->trigger();
    check(tabs->currentIndex() == 1, "direct keyboard Graphing navigation");
    auto editors = w.findChildren<MathExpressionEdit*>();
    int before = 0;
    for (const auto* e : editors)
        if (e->objectName().startsWith("GraphExpression_")) ++before;
    add->trigger();
    int after = 0;
    MathExpressionEdit* last = nullptr;
    for (auto* e : w.findChildren<MathExpressionEdit*>()) {
        if (e->objectName().startsWith("GraphExpression_")) { ++after; last = e; }
    }
    check(after == before + 1, "Ctrl+N action appends one Cartesian row");
    check(last && last->hasFocus(), "new graph row receives keyboard focus");
    tab3->trigger();
    check(tabs->currentIndex() == 2, "direct Exact Fractions navigation");
    focus->trigger();
    check(w.focusWidget() && qobject_cast<QLineEdit*>(w.focusWidget()) != nullptr,
          "fraction input receives focus");
    add->trigger();
    int notOnGraph = 0;
    for (auto* e : w.findChildren<MathExpressionEdit*>())
        if (e->objectName().startsWith("GraphExpression_")) ++notOnGraph;
    check(notOnGraph == after, "graph create command inactive outside Graphing");

    tab4->trigger();
    check(tabs->currentIndex() == 3, "direct Advanced Math navigation");
    auto* inner = w.findChild<QTabWidget*>("advancedMathTabs");
    check(inner && inner->count() == 6, "six advanced mathematical panels preserved");
    if (inner) {
        const int current = inner->currentIndex();
        nextAdvanced->trigger();
        check(inner->currentIndex() == (current + 1) % inner->count(),
              "next advanced panel selection");
        check(w.focusWidget() && qobject_cast<QLineEdit*>(w.focusWidget()) != nullptr,
              "advanced input receives focus");
        priorAdvanced->trigger();
        check(inner->currentIndex() == current, "previous advanced panel selection");
    }
    tab1->trigger();
    check(tabs->currentIndex() == 0 && input->hasFocus(),
          "scientific tab navigation restores editor focus");
    check(input->text() == "12+3", "tab navigation does not mutate expression");
    next->trigger();
    check(tabs->currentIndex() == 1, "next tab selection");
    prior->trigger();
    check(tabs->currentIndex() == 0, "previous tab selection");
    prior->trigger();
    check(tabs->currentIndex() == 3, "previous tab wraps at first tab");
    next->trigger();
    check(tabs->currentIndex() == 0, "next tab wraps at last tab");

    graphFocus->trigger();
    check(tabs->currentIndex() == 1 && last && last->hasFocus(),
          "focus graph expression regardless of current tab");
    action(w, "keyboardGraphHome")->trigger();
    check(tabs->currentIndex() == 1, "graph Home action preserves graph tab");

    // Local mathematical input shortcuts retain precedence and behavior.
    tab1->trigger();
    input->setText("x");
    input->setCursorPosition(1);
    press(input, Qt::Key_P, Qt::AltModifier);
    check(input->text() == QString::fromUtf8("xπ"), "Alt+P local math symbol survives");
    check(action(w, "keyboardShortcutsHelp") != nullptr, "help accelerator exists");
    action(w, "keyboardShortcutsHelp")->trigger();
    app.processEvents();
    auto* dialog = w.findChild<QDialog*>("keyboardShortcutsDialog");
    check(dialog && dialog->isVisible(), "keyboard help is displayed without blocking");
    auto* reference = dialog ? dialog->findChild<QTextBrowser*>("keyboardShortcutsReference") : nullptr;
    check(reference && reference->toPlainText().contains("Alt+Up/Down"),
          "help explains existing graph row shortcuts");
    if (dialog) { dialog->close(); app.processEvents(); }

    w.close();
    std::cout << (failures ? "FAIL: " : "PASS: ") << checks
              << " KeyboardNavigationGui assertions\n";
    return failures ? 1 : 0;
}
