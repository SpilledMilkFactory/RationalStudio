#include "ApplicationSettings.hpp"
#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <iostream>

using namespace rationalstudio;
namespace {
int count = 0, failed = 0;
void check(bool ok, const char* name) {
    ++count;
    if (!ok) { ++failed; std::cerr << "FAIL: " << name << '\n'; }
}
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir temp;
    check(temp.isValid(), "temporary settings directory");
    if (!temp.isValid()) return 2;
    const QString path = temp.filePath("prefs.ini");
    {
        QSettings store(path, QSettings::IniFormat);
        auto prefs = readApplicationSettings(store);
        check(prefs.angleMode == AngleMode::Radians, "default RAD");
        check(prefs.appearance == AppearanceMode::Dark, "default dark");
        check(prefs.showGraphGrid && prefs.showGraphAxes, "default grid and axes");
        check(!prefs.rememberLastTab && prefs.lastTab == 0, "default startup tab");
        check(prefs.windowGeometry.isEmpty(), "no default geometry");
        prefs.angleMode = AngleMode::Degrees;
        prefs.appearance = AppearanceMode::Light;
        prefs.showGraphGrid = false;
        prefs.showGraphAxes = false;
        prefs.rememberLastTab = true;
        prefs.lastTab = 3;
        prefs.windowGeometry = QByteArray("binary\0blob", 11);
        writeApplicationSettings(store, prefs);
        check(store.status() == QSettings::NoError, "settings write status");
    }
    {
        QSettings store(path, QSettings::IniFormat);
        const auto prefs = readApplicationSettings(store);
        check(prefs.angleMode == AngleMode::Degrees, "persist DEG");
        check(prefs.appearance == AppearanceMode::Light, "persist light");
        check(!prefs.showGraphGrid && !prefs.showGraphAxes, "persist grid and axes");
        check(prefs.rememberLastTab && prefs.lastTab == 3, "persist last tab");
        check(prefs.windowGeometry == QByteArray("binary\0blob", 11), "binary geometry round trip");
        store.setValue("ui/angleMode", "nonsense");
        store.setValue("ui/appearance", "other");
        store.setValue("graph/showGrid", "garbage");
        store.setValue("graph/showAxes", "garbage");
        store.setValue("ui/rememberTab", "garbage");
        store.setValue("ui/lastTab", "999999");
        store.setValue("ui/windowGeometry", QByteArray(65537, 'X'));
        store.sync();
        const auto repaired = readApplicationSettings(store);
        check(repaired.angleMode == AngleMode::Radians, "corrupt angle defaults");
        check(repaired.appearance == AppearanceMode::Dark, "corrupt appearance defaults");
        check(repaired.showGraphGrid && repaired.showGraphAxes, "corrupt toggles default");
        check(!repaired.rememberLastTab && repaired.lastTab == 0, "corrupt navigation defaults");
        check(repaired.windowGeometry.isEmpty(), "oversize geometry rejected");
        ApplicationSettings defaults;
        defaults.lastTab = 3;
        writeApplicationSettings(store, defaults);
        check(store.value("ui/lastTab").toInt() == 0, "disabled tab restore not stored");
    }
    std::cout << (failed ? "FAIL: " : "PASS: ") << count
              << " ApplicationSettings assertions\n";
    return failed ? 1 : 0;
}
