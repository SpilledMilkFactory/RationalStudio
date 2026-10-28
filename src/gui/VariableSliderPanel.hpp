#pragma once

#include "VariableDefinitions.hpp"
#include "VariableSliderScale.hpp"
#include "GraphWorkspacePersistence.hpp"

#include <QWidget>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QSlider;
class QString;
class QVBoxLayout;

namespace rationalstudio {

// Session-local controls; Issue #31 connects them to graph bindings and
// selective repaint. Workspace persistence is deferred to Issue #35.
class VariableSliderPanel final : public QWidget {
public:
    explicit VariableSliderPanel(QWidget* parent = nullptr);

    bool addVariable(const std::string& name, double initial = 0,
                     double minimum = -10, double maximum = 10,
                     double step = 0.5, std::string* error = nullptr);
    bool configureVariable(const std::string& name, double minimum,
                           double maximum, double step,
                           std::string* error = nullptr);
    bool setVariableValue(const std::string& name, double value,
                          std::string* error = nullptr);
    bool removeVariable(const std::string& name, std::string* error = nullptr);

    std::optional<double> value(const std::string& name) const {
        return definitions_.value(name);
    }
    VariableDefinitions& definitions() noexcept { return definitions_; }
    const VariableDefinitions& definitions() const noexcept { return definitions_; }
    std::size_t rowCount() const noexcept { return rows_.size(); }
    [[nodiscard]] std::vector<WorkspaceSlider> snapshotSliders() const;
    // Only call with a fully validated, staged workspace. UI notifications
    // are coalesced so no intermediate state can reach GraphCanvas.
    bool replaceWorkspace(const std::vector<WorkspaceSlider>& sliders,
                          const VariableDefinitions& validated, std::string* error = nullptr);
    std::function<bool(const std::string&, std::string*)> mayAddVariable;
    std::function<bool(const std::string&, std::string*)> mayRemoveVariable;

    // Called only after a successful value change, including dependent updates.
    std::function<void(const std::string&, double)> onValueChanged;
    // Called on successful mutations. For a value-only edit, changed contains
    // exactly the scalar definitions whose evaluated result changed. For
    // addition/removal, scopeChanged is true and changed includes the name.
    // Notify after committing the authoritative store, never after rejection.
    std::function<void(const std::vector<std::string>& changed,
                       bool scopeChanged)> onDefinitionsChanged;

private:
    struct Row {
        std::string name;
        VariableSliderScale scale;
        QWidget* container = nullptr;
        QSlider* slider = nullptr;
        QDoubleSpinBox* current = nullptr;
        QDoubleSpinBox* minimum = nullptr;
        QDoubleSpinBox* maximum = nullptr;
        QDoubleSpinBox* step = nullptr;
        QLabel* status = nullptr;
    };

    Row* findRow(const std::string& name) const;
    void syncRow(Row& row);
    void showMessage(const QString& message, bool error);

    VariableDefinitions definitions_;
    std::vector<std::unique_ptr<Row>> rows_;
    QLineEdit* nameInput_ = nullptr;
    QVBoxLayout* list_ = nullptr;
    QLabel* message_ = nullptr;
    bool restoring_ = false;
};

} // namespace rationalstudio
