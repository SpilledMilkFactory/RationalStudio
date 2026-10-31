#include "MainWindow.hpp"
#include "GraphCanvas.hpp"
#include "MathExpressionEdit.hpp"
#include <QAction>
#include <QApplication>
#include <QSettings>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <iostream>

using namespace rationalstudio;
namespace {
int count = 0, failed = 0;
void check(bool ok, const char* name) {
    ++count;
    if (!ok) { ++failed; std::cerr << "FAIL: " << name << '\n'; }
}
GraphCanvas* graphCanvas(QWidget& w) {
    for (auto* widget : w.findChildren<QWidget*>())
        if (auto* canvas = dynamic_cast<GraphCanvas*>(widget)) return canvas;
    return nullptr;
}
QAction* namedAction(QWidget& w, const char* name) {
    return w.findChild<QAction*>(QString::fromLatin1(name));
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QTemporaryDir temp;
    check(temp.isValid(), "temporary GUI settings directory");
    if (!temp.isValid()) return 2;
    const QString path = temp.filePath("rationalstudio-gui.ini");
    {
        MainWindow w(nullptr, path);
        w.show();
        QApplication::processEvents();
        auto* tabs = w.findChild<QTabWidget*>();
        auto* canvas = graphCanvas(w);
        auto* dark = namedAction(w, "preferenceDark");
        auto* light = namedAction(w, "preferenceLight");
        auto* grid = namedAction(w, "preferenceGraphGrid");
        auto* axes = namedAction(w, "preferenceGraphAxes");
        auto* remember = namedAction(w, "preferenceRememberTab");
        check(tabs && tabs->count() == 4, "four original tab positions");
        check(canvas && !canvas->lightAppearance(), "default graph palette");
        check(dark && dark->isChecked(), "default dark action");
        check(light && grid && axes && remember, "preference actions available");
        if (!tabs || !canvas || !light || !grid || !axes || !remember) return 2;
        light->trigger();
        check(light->isChecked() && canvas->lightAppearance(), "light applies immediately");
        grid->trigger(); axes->trigger(); remember->trigger();
        check(!grid->isChecked() && !axes->isChecked(), "graph controls updated");
        tabs->setCurrentIndex(3);
        check(!canvas->gridVisible() && !canvas->axesVisible(), "graph palette toggles applied");
        QPushButton* degrees = nullptr;
        for (auto* button : w.findChildren<QPushButton*>())
            if (button->text() == "DEG") { degrees = button; break; }
        check(degrees != nullptr, "degree mode action available");
        if (degrees) degrees->click();
        w.resize(1180, 730);
        w.close();
    }
    {
        QSettings persisted(path, QSettings::IniFormat);
        check(persisted.value("ui/appearance") == "light", "appearance written on close");
        check(persisted.value("ui/angleMode") == "degrees", "angle written on close");
        check(persisted.value("graph/showGrid").toString() == "false", "grid written on close");
        check(persisted.value("ui/rememberTab").toString() == "true", "remember tab written on close");
    }
    {
        MainWindow w(nullptr, path);
        auto* tabs = w.findChild<QTabWidget*>();
        auto* canvas = graphCanvas(w);
        auto* light = namedAction(w, "preferenceLight");
        check(tabs && tabs->currentIndex() == 3, "restore last tab");
        check(canvas && canvas->lightAppearance(), "restore graph palette");
        check(canvas && !canvas->gridVisible() && !canvas->axesVisible(), "restore graph toggles");
        QPushButton* degrees = nullptr;
        for (auto* button : w.findChildren<QPushButton*>())
            if (button->text() == "DEG") { degrees = button; break; }
        check(degrees && degrees->isChecked(), "restore DEG mode");
        check(light && light->isChecked(), "restore appearance action");
        auto* remember = namedAction(w, "preferenceRememberTab");
        if (remember) remember->trigger();
        w.close();
    }
    {
        MainWindow w(nullptr, path);
        auto* tabs = w.findChild<QTabWidget*>();
        check(tabs && tabs->currentIndex() == 0, "disabled startup tab restore");
        w.close();
    }
    std::cout << (failed ? "FAIL: " : "PASS: ") << count
              << " ApplicationSettingsGui assertions\n";
    return failed ? 1 : 0;
}
