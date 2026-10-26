#pragma once

#include "VariableDefinitions.hpp"
#include "VariableSliderScale.hpp"

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

// Session-local controls; graph bindings, automatic selective repaint and
// persistence are explicitly deferred to Issues #31 and #35.
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

    // Called only after a successful value change, including dependent updates.
    std::function<void(const std::string&, double)> onValueChanged;

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
};

} // namespace rationalstudio
