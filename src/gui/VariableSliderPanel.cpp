#include "VariableSliderPanel.hpp"

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace rationalstudio {
namespace {

QString displayName(const std::string& name) {
    return QString::fromStdString(name);
}

std::string numericSource(const std::string& name, double value) {
    std::ostringstream out;
    out << name << " = " << std::setprecision(std::numeric_limits<double>::max_digits10)
        << value;
    return out.str();
}

void setError(std::string* destination, const std::string& message) {
    if (destination) *destination = message;
}

QDoubleSpinBox* numericBox(QWidget* parent) {
    auto* spin = new QDoubleSpinBox(parent);
    spin->setRange(-1000000000.0, 1000000000.0);
    spin->setDecimals(9);
    spin->setSingleStep(0.5);
    spin->setKeyboardTracking(false);
    spin->setMinimumWidth(64);
    return spin;
}

} // namespace

VariableSliderPanel::VariableSliderPanel(QWidget* parent)
    : QWidget(parent) {
    setObjectName("variableSliderPanel");
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 2, 0, 2);
    outer->setSpacing(6);

    auto* header = new QLabel("Variables · saved in graph workspaces", this);
    header->setStyleSheet("font-weight:bold; color:#92ddc0;");
    outer->addWidget(header);

    auto* addLine = new QHBoxLayout;
    nameInput_ = new QLineEdit(this);
    nameInput_->setObjectName("newVariableName");
    nameInput_->setPlaceholderText("Name (e.g. a)");
    nameInput_->setMaxLength(64);
    nameInput_->setToolTip("Create a new scalar variable with range -10 to 10 and step 0.5");
    auto* add = new QPushButton("+ Slider", this);
    add->setObjectName("addVariableSlider");
    addLine->addWidget(nameInput_, 1);
    addLine->addWidget(add);
    outer->addLayout(addLine);
    connect(add, &QPushButton::clicked, this, [this] {
        std::string error;
        if (addVariable(nameInput_->text().trimmed().toStdString(), 0, -10, 10, 0.5, &error)) {
            nameInput_->clear();
        } else {
            showMessage(QString::fromStdString(error), true);
        }
    });
    connect(nameInput_, &QLineEdit::returnPressed, add, &QPushButton::click);

    auto* group = new QGroupBox("Adjust values and ranges", this);
    list_ = new QVBoxLayout(group);
    list_->setContentsMargins(5, 7, 5, 5);
    list_->setSpacing(8);
    outer->addWidget(group);
    message_ = new QLabel(
        "Slider changes refresh affected graph curves. "
        "Workspace Save/Open preserves slider settings.", this);
    message_->setObjectName("sliderMessage");
    message_->setWordWrap(true);
    message_->setStyleSheet("color:#9aacbf; font-size:11px;");
    outer->addWidget(message_);
}

VariableSliderPanel::Row* VariableSliderPanel::findRow(const std::string& name) const {
    // 'names' is canonical lowercase in VariableDefinitions; identifier
    // validation is performed by core before any new row is inserted.
    VariableScope scope;
    if (!scope.declare(name)) return nullptr;
    const auto canonical = *scope.names().begin();
    for (const auto& row : rows_) {
        if (row->name == canonical) return row.get();
    }
    return nullptr;
}

void VariableSliderPanel::showMessage(const QString& message, bool error) {
    message_->setText(message);
    message_->setStyleSheet(error
        ? "color:#ff9299; font-size:11px;"
        : "color:#9ad9c2; font-size:11px;");
}

bool VariableSliderPanel::addVariable(
    const std::string& name, double initial, double minimum,
    double maximum, double step, std::string* error) {
    VariableScope valid;
    if (!valid.declare(name, error)) return false;
    const std::string canonical = *valid.names().begin();
    if (mayAddVariable && !mayAddVariable(canonical, error)) return false;
    if (definitions_.contains(canonical)) {
        setError(error, "Variable already exists");
        return false;
    }
    auto scale = VariableSliderScale::create(minimum, maximum, step, error);
    if (!scale) return false;
    if (!std::isfinite(initial) || initial < minimum || initial > maximum) {
        setError(error, "Initial value must be finite and inside the range");
        return false;
    }
    // The GUI's QDoubleSpinBox precision and supported range are intentionally
    // narrower than the mathematical core's finite-double domain.
    if (minimum < -1e9 || maximum > 1e9 || step < 1e-9 ||
        (initial != 0 && std::abs(initial) < 1e-9)) {
        setError(error, "Slider controls require values within ±1e9 and step >= 1e-9");
        return false;
    }
    initial = scale->valueAt(scale->nearestTick(initial));
    const VariableUpdate result = definitions_.assign(numericSource(canonical, initial));
    if (!result.accepted) {
        setError(error, result.error);
        return false;
    }
    auto row = std::make_unique<Row>();
    row->name = canonical;
    row->scale = *scale;
    row->container = new QWidget(this);
    row->container->setObjectName("variableRow_" + displayName(canonical));
    auto* layout = new QVBoxLayout(row->container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(3);
    auto* title = new QHBoxLayout;
    auto* nameLabel = new QLabel(displayName(canonical), row->container);
    nameLabel->setStyleSheet("font-weight:bold;");
    title->addWidget(nameLabel);
    row->current = numericBox(row->container);
    row->current->setObjectName("variableValue_" + displayName(canonical));
    title->addWidget(row->current, 1);
    auto* remove = new QToolButton(row->container);
    remove->setText("×");
    remove->setObjectName("removeVariable_" + displayName(canonical));
    remove->setToolTip("Remove variable (only if no definitions depend on it)");
    title->addWidget(remove);
    layout->addLayout(title);
    row->slider = new QSlider(Qt::Horizontal, row->container);
    row->slider->setObjectName("variableSlider_" + displayName(canonical));
    layout->addWidget(row->slider);
    auto* limits = new QGridLayout;
    row->minimum = numericBox(row->container);
    row->maximum = numericBox(row->container);
    row->step = numericBox(row->container);
    row->step->setRange(0.000000001, 1000000000.0);
    row->minimum->setObjectName("variableMin_" + displayName(canonical));
    row->maximum->setObjectName("variableMax_" + displayName(canonical));
    row->step->setObjectName("variableStep_" + displayName(canonical));
    limits->addWidget(new QLabel("Min", row->container), 0, 0);
    limits->addWidget(new QLabel("Max", row->container), 0, 1);
    limits->addWidget(new QLabel("Step", row->container), 0, 2);
    limits->addWidget(row->minimum, 1, 0);
    limits->addWidget(row->maximum, 1, 1);
    limits->addWidget(row->step, 1, 2);
    layout->addLayout(limits);
    row->status = new QLabel(row->container);
    row->status->setWordWrap(true);
    row->status->setStyleSheet("color:#9aadbf; font-size:10px;");
    layout->addWidget(row->status);

    Row* pointer = row.get();
    rows_.push_back(std::move(row));
    list_->addWidget(pointer->container);
    syncRow(*pointer);

    connect(pointer->slider, &QSlider::valueChanged, this,
        [this, pointer](int tick) {
            std::string message;
            if (!setVariableValue(pointer->name, pointer->scale.valueAt(tick), &message))
                showMessage(QString::fromStdString(message), true);
        });
    connect(pointer->current, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
        [this, pointer](double value) {
            std::string message;
            if (!setVariableValue(pointer->name, value, &message))
                showMessage(QString::fromStdString(message), true);
        });
    auto configure = [this, pointer] {
        std::string message;
        if (!configureVariable(pointer->name, pointer->minimum->value(),
                               pointer->maximum->value(), pointer->step->value(), &message)) {
            pointer->status->setText(QString::fromStdString(message));
            pointer->status->setStyleSheet("color:#ff9299; font-size:10px;");
        }
    };
    connect(pointer->minimum, qOverload<double>(&QDoubleSpinBox::valueChanged), this, configure);
    connect(pointer->maximum, qOverload<double>(&QDoubleSpinBox::valueChanged), this, configure);
    connect(pointer->step, qOverload<double>(&QDoubleSpinBox::valueChanged), this, configure);
    connect(remove, &QToolButton::clicked, this, [this, canonical] {
        std::string message;
        if (!removeVariable(canonical, &message))
            showMessage(QString::fromStdString(message), true);
    });
    setError(error, {});
    showMessage("Added slider for " + displayName(canonical), false);
    if (!restoring_ && onDefinitionsChanged) onDefinitionsChanged({canonical}, true);
    return true;
}

void VariableSliderPanel::syncRow(Row& row) {
    const auto v = definitions_.value(row.name);
    if (!v) return;
    const QSignalBlocker blockSlider(row.slider);
    const QSignalBlocker blockCurrent(row.current);
    const QSignalBlocker blockMin(row.minimum);
    const QSignalBlocker blockMax(row.maximum);
    const QSignalBlocker blockStep(row.step);
    row.slider->setRange(0, row.scale.ticks());
    row.slider->setValue(row.scale.nearestTick(*v));
    row.current->setValue(*v);
    row.minimum->setValue(row.scale.minimum());
    row.maximum->setValue(row.scale.maximum());
    row.step->setValue(row.scale.step());
    row.status->setText("Current: " + QString::number(*v, 'g', 12));
    row.status->setStyleSheet("color:#9aadbf; font-size:10px;");
}

bool VariableSliderPanel::setVariableValue(
    const std::string& name, double value, std::string* error) {
    Row* row = findRow(name);
    if (!row) { setError(error, "Unknown slider variable"); return false; }
    if (!std::isfinite(value)) {
        setError(error, "Value must be finite");
        return false;
    }
    const double snapped = row->scale.valueAt(row->scale.nearestTick(value));
    const VariableUpdate update = definitions_.assign(numericSource(row->name, snapped));
    if (!update.accepted) {
        setError(error, update.error);
        syncRow(*row);
        return false;
    }
    syncRow(*row);
    setError(error, {});
    if (!restoring_ && !update.changed.empty()) {
        if (onDefinitionsChanged) onDefinitionsChanged(update.changed, false);
        if (onValueChanged) onValueChanged(row->name, snapped);
    }
    return true;
}

bool VariableSliderPanel::configureVariable(
    const std::string& name, double minimum, double maximum,
    double step, std::string* error) {
    Row* row = findRow(name);
    if (!row) { setError(error, "Unknown slider variable"); return false; }
    auto proposed = VariableSliderScale::create(minimum, maximum, step, error);
    if (!proposed) return false;
    if (minimum < -1e9 || maximum > 1e9 || step < 1e-9) {
        setError(error, "Slider controls require endpoints within ±1e9 and step >= 1e-9");
        return false;
    }
    const double oldValue = definitions_.value(row->name).value_or(0);
    const double clamped = std::clamp(oldValue, minimum, maximum);
    const double snapped = proposed->valueAt(proposed->nearestTick(clamped));
    const VariableUpdate update = definitions_.assign(numericSource(row->name, snapped));
    if (!update.accepted) { setError(error, update.error); return false; }
    row->scale = *proposed;
    syncRow(*row);
    setError(error, {});
    if (!restoring_ && !update.changed.empty()) {
        if (onDefinitionsChanged) onDefinitionsChanged(update.changed, false);
        if (onValueChanged) onValueChanged(row->name, snapped);
    }
    return true;
}

bool VariableSliderPanel::removeVariable(const std::string& name, std::string* error) {
    Row* row = findRow(name);
    if (!row) { setError(error, "Unknown slider variable"); return false; }
    if (mayRemoveVariable && !mayRemoveVariable(row->name, error)) return false;
    const VariableUpdate update = definitions_.erase(row->name);
    if (!update.accepted) { setError(error, update.error); return false; }
    const auto it = std::find_if(rows_.begin(), rows_.end(),
        [row](const auto& ptr) { return ptr.get() == row; });
    if (it != rows_.end()) {
        QObject::disconnect(row->slider, nullptr, this, nullptr);
        QObject::disconnect(row->current, nullptr, this, nullptr);
        QObject::disconnect(row->minimum, nullptr, this, nullptr);
        QObject::disconnect(row->maximum, nullptr, this, nullptr);
        QObject::disconnect(row->step, nullptr, this, nullptr);
        row->container->deleteLater();
        rows_.erase(it);
    }
    showMessage("Removed slider for " + displayName(name), false);
    setError(error, {});
    if (!restoring_ && onDefinitionsChanged) onDefinitionsChanged({name}, true);
    return true;
}

std::vector<WorkspaceSlider> VariableSliderPanel::snapshotSliders() const {
    std::vector<WorkspaceSlider> result;
    result.reserve(rows_.size());
    for (const auto& row : rows_) {
        result.push_back({row->name, definitions_.value(row->name).value_or(0),
                          row->scale.minimum(), row->scale.maximum(), row->scale.step()});
    }
    return result;
}

bool VariableSliderPanel::replaceWorkspace(
    const std::vector<WorkspaceSlider>& sliders,
    const VariableDefinitions& validated, std::string* error) {
    // This is invoked only after GraphWorkspacePersistence has checked the
    // complete document, all scalar dependencies and slider constraints.
    // Keep the mathematical owner available for all restored callbacks.
    restoring_ = true;
    for (auto& row : rows_) {
        // Deleting immediately avoids duplicate object names during reload.
        delete row->container;
    }
    rows_.clear();
    definitions_ = VariableDefinitions(validated.angleMode());
    for (const auto& slider : sliders) {
        if (!addVariable(slider.name, slider.value, slider.minimum,
                         slider.maximum, slider.step, error)) {
            restoring_ = false;
            return false;
        }
    }
    definitions_ = validated;
    for (auto& row : rows_) syncRow(*row);
    restoring_ = false;
    showMessage("Workspace variables restored", false);
    if (onDefinitionsChanged) onDefinitionsChanged(definitions_.names(), true);
    if (error) error->clear();
    return true;
}

} // namespace rationalstudio
