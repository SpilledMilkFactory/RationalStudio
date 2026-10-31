#include "ApplicationSettings.hpp"

#include <QSettings>
#include <QString>
#include <QVariant>
#include <limits>

namespace rationalstudio {
namespace {
// QSettings converters are deliberately strict: do not interpret arbitrary
// strings or bad preferences as a user instruction.
bool readBool(const QSettings& settings, const QString& key, bool fallback) {
    const QVariant value = settings.value(key);
    if (!value.isValid()) return fallback;
    const QString s = value.toString().trimmed().toLower();
    if (s == "true" || s == "1") return true;
    if (s == "false" || s == "0") return false;
    return fallback;
}
int readInt(const QSettings& settings, const QString& key, int fallback, int max) {
    const QString text = settings.value(key).toString();
    if (text.isEmpty() || text.trimmed() != text) return fallback;
    bool ok = false;
    const int n = text.toInt(&ok);
    return ok && n >= 0 && n <= max ? n : fallback;
}
} // namespace

ApplicationSettings readApplicationSettings(const QSettings& settings) {
    ApplicationSettings out;
    const QString angle = settings.value("ui/angleMode").toString();
    if (angle == "degrees") out.angleMode = AngleMode::Degrees;
    const QString appearance = settings.value("ui/appearance").toString();
    if (appearance == "light") out.appearance = AppearanceMode::Light;
    out.showGraphGrid = readBool(settings, "graph/showGrid", true);
    out.showGraphAxes = readBool(settings, "graph/showAxes", true);
    out.rememberLastTab = readBool(settings, "ui/rememberTab", false);
    out.lastTab = readInt(settings, "ui/lastTab", 0, 3);
    const QByteArray geometry = settings.value("ui/windowGeometry").toByteArray();
    if (geometry.size() <= 65536) out.windowGeometry = geometry;
    return out;
}

void writeApplicationSettings(QSettings& settings, const ApplicationSettings& values) {
    settings.setValue("schema", 1);
    settings.setValue("ui/angleMode", values.angleMode == AngleMode::Degrees ? "degrees" : "radians");
    settings.setValue("ui/appearance", values.appearance == AppearanceMode::Light ? "light" : "dark");
    settings.setValue("graph/showGrid", values.showGraphGrid);
    settings.setValue("graph/showAxes", values.showGraphAxes);
    settings.setValue("ui/rememberTab", values.rememberLastTab);
    settings.setValue("ui/lastTab", values.rememberLastTab && values.lastTab >= 0 && values.lastTab <= 3 ? values.lastTab : 0);
    if (values.windowGeometry.size() <= 65536)
        settings.setValue("ui/windowGeometry", values.windowGeometry);
    settings.sync();
}
} // namespace rationalstudio
