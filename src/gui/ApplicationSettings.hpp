#pragma once

#include "Expression.hpp"
#include <QByteArray>

class QSettings;
namespace rationalstudio {

// User preferences, deliberately separate from GraphWorkspacePersistence.
// Stored with QSettings; malformed/unknown fields fall back to safe defaults.
enum class AppearanceMode { Dark, Light };
struct ApplicationSettings {
    AngleMode angleMode = AngleMode::Radians;
    AppearanceMode appearance = AppearanceMode::Dark;
    bool showGraphGrid = true;
    bool showGraphAxes = true;
    bool rememberLastTab = false;
    int lastTab = 0;
    QByteArray windowGeometry;
};
[[nodiscard]] ApplicationSettings readApplicationSettings(const QSettings& settings);
void writeApplicationSettings(QSettings& settings, const ApplicationSettings& values);
} // namespace rationalstudio
