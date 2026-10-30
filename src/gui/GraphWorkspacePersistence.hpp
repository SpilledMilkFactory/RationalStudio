#pragma once

// Qt-dependent JSON boundary. Mathematical definitions are validated using
// the existing Qt-independent owners before the GUI state can be committed.
#include "VariableDefinitions.hpp"
#include "FunctionDefinitions.hpp"
#include "VariableSliderScale.hpp"
#include "MathExpressionInput.hpp"
#include "GraphViewport.hpp"
#include "ParametricSampler.hpp"
#include "PolarExpression.hpp"
#include "ImplicitContour.hpp"
#include "InequalityRegion.hpp"

#include <QByteArray>
#include <QColor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace rationalstudio {

struct WorkspaceSlider {
    std::string name;
    double value = 0, minimum = -10, maximum = 10, step = 0.5;
};
struct WorkspaceAssignment { std::string name, expression; };
struct WorkspaceFunction { std::string name, body; std::vector<std::string> parameters; };
struct WorkspaceCurve {
    QString expression;
    bool visible = true;
    QColor color;
    bool parametric = false;
    QString yExpression{};
    double tMinimum = 0;
    double tMaximum = 0;
    bool polar = false;
    bool implicit = false;
    bool inequality = false;
};

struct GraphWorkspaceState {
    double centerX = 0, centerY = 0, pixelsPerUnit = 58;
    AngleMode angle = AngleMode::Radians;
    std::vector<WorkspaceCurve> curves;
    std::vector<WorkspaceSlider> sliders;
    std::vector<WorkspaceAssignment> assignments; // non-slider scalar definitions, ordered by dependency
    std::vector<WorkspaceFunction> functions;      // dependency-first
    VariableDefinitions validatedVariables;
    FunctionDefinitions validatedFunctions;
};

class GraphWorkspacePersistence final {
public:
    static constexpr qint64 MaxFileBytes = 1024 * 1024;

    // Uses the dependency metadata, not alphabetical order, to produce
    // deterministic definitions that can be restored without forward refs.
    static std::vector<WorkspaceAssignment> orderedAssignments(
        const VariableDefinitions& store, const std::vector<WorkspaceSlider>& sliders) {
        std::set<std::string> sliderNames;
        for (const auto& slider : sliders) sliderNames.insert(slider.name);
        std::vector<WorkspaceAssignment> ordered;
        std::set<std::string> visited;
        std::function<void(const std::string&)> visit = [&](const std::string& name) {
            if (!visited.insert(name).second) return;
            for (const auto& dep : store.directDependencies(name)) {
                if (!sliderNames.contains(dep)) visit(dep);
            }
            if (!sliderNames.contains(name)) ordered.push_back({name, store.source(name)});
        };
        for (const auto& name : store.names()) visit(name);
        return ordered;
    }

    static std::vector<WorkspaceFunction> orderedFunctions(const FunctionDefinitions& store) {
        std::vector<WorkspaceFunction> ordered;
        std::set<std::string> visited;
        std::function<void(const std::string&)> visit = [&](const std::string& name) {
            if (!visited.insert(name).second) return;
            for (const auto& dep : store.directFunctionDependencies(name)) visit(dep);
            ordered.push_back({name, store.source(name), store.parameters(name)});
        };
        for (const auto& name : store.names()) visit(name);
        return ordered;
    }

    static QByteArray encode(const GraphWorkspaceState& state) {
        QJsonObject root;
        root["format"] = "RationalStudioGraph";
        const bool hasParametric = std::any_of(state.curves.begin(), state.curves.end(),
            [](const WorkspaceCurve& row) { return row.parametric || row.polar || row.implicit || row.inequality; });
        root["version"] = hasParametric ? 3 : 2;
        root["centerX"] = state.centerX;
        root["centerY"] = state.centerY;
        root["pixelsPerUnit"] = state.pixelsPerUnit;
        root["angleMode"] = state.angle == AngleMode::Degrees ? "degrees" : "radians";
        QJsonArray expressions;
        for (const auto& row : state.curves) {
            QJsonObject item{{"expression", row.expression},
                             {"visible", row.visible},
                             {"color", row.color.name()}};
            if (hasParametric) item["kind"] = row.inequality ? "inequality" : (row.implicit ? "implicit" : (row.polar ? "polar" :
                (row.parametric ? "parametric" : "cartesian")));
            if (row.polar) {
                item["tMin"] = row.tMinimum;
                item["tMax"] = row.tMaximum;
            }
            if (row.parametric) {
                item["yExpression"] = row.yExpression;
                item["tMin"] = row.tMinimum;
                item["tMax"] = row.tMaximum;
            }
            expressions.append(item);
        }
        root["expressions"] = expressions;
        QJsonArray sliders;
        for (const auto& row : state.sliders) {
            sliders.append(QJsonObject{{"name", QString::fromStdString(row.name)},
                                       {"value", row.value}, {"minimum", row.minimum},
                                       {"maximum", row.maximum}, {"step", row.step}});
        }
        root["sliders"] = sliders;
        QJsonArray variables;
        for (const auto& row : state.assignments) {
            variables.append(QJsonObject{{"name", QString::fromStdString(row.name)},
                                         {"expression", QString::fromStdString(row.expression)}});
        }
        root["variables"] = variables;
        QJsonArray functions;
        for (const auto& row : state.functions) {
            QJsonArray parameters;
            for (const auto& p : row.parameters) parameters.append(QString::fromStdString(p));
            functions.append(QJsonObject{{"name", QString::fromStdString(row.name)},
                                         {"parameters", parameters},
                                         {"body", QString::fromStdString(row.body)}});
        }
        root["functions"] = functions;
        return QJsonDocument(root).toJson(QJsonDocument::Indented);
    }

    static bool decode(const QByteArray& bytes, GraphWorkspaceState& output, QString* error = nullptr) {
        const auto reject = [&](const QString& why) {
            if (error) *error = why;
            return false;
        };
        if (bytes.size() > MaxFileBytes) return reject("Workspace is larger than the 1 MiB limit.");
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(bytes, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
            return reject("Invalid JSON graph workspace.");
        const auto root = document.object();
        if (root.value("format").toString() != "RationalStudioGraph" ||
            !root.value("version").isDouble()) return reject("Unsupported workspace format.");
        const int version = root.value("version").toInt(-1);
        if ((version != 1 && version != 2 && version != 3) || !root.value("expressions").isArray())
            return reject("Unsupported graph workspace version or expression list.");
        const auto curves = root.value("expressions").toArray();
        if (curves.size() > 40) return reject("Workspace contains more than 40 curves.");
        GraphWorkspaceState stage;
        for (const auto& key : {"centerX", "centerY", "pixelsPerUnit"}) {
            if (root.contains(key) && !root.value(key).isDouble())
                return reject(QString("Invalid %1 setting.").arg(key));
        }
        stage.centerX = root.value("centerX").toDouble(0);
        stage.centerY = root.value("centerY").toDouble(0);
        stage.pixelsPerUnit = root.value("pixelsPerUnit").toDouble(58);
        GraphViewport viewport;
        if (!viewport.setView(stage.centerX, stage.centerY, stage.pixelsPerUnit))
            return reject("Invalid viewport coordinates or scale.");
        if (root.contains("angleMode") && !root.value("angleMode").isString())
            return reject("Invalid angle mode.");
        const auto angle = root.value("angleMode").toString("radians");
        if (angle != "radians" && angle != "degrees") return reject("Unknown angle mode.");
        stage.angle = angle == "degrees" ? AngleMode::Degrees : AngleMode::Radians;
        stage.validatedVariables.setAngleMode(stage.angle);
        for (const auto& item : curves) {
            if (!item.isObject()) return reject("Curve entry is not an object.");
            const auto obj = item.toObject();
            if (!obj.value("expression").isString() ||
                obj.value("expression").toString().size() > 2048 ||
                (obj.contains("visible") && !obj.value("visible").isBool()) ||
                (obj.contains("color") && !obj.value("color").isString()))
                return reject("Invalid curve entry.");
            const auto colorString = obj.value("color").toString("#50c4ef");
            const QColor color(colorString);
            if (!color.isValid()) return reject("Invalid curve color.");
            WorkspaceCurve row{obj.value("expression").toString(),
                               obj.value("visible").toBool(true), color};
            if (version == 3) {
                if (!obj.value("kind").isString()) return reject("Missing curve representation kind.");
                const auto kind = obj.value("kind").toString();
                if (kind == "inequality") {
                    row.inequality = true;
                    if (!parseInequality(row.expression.toStdString()).valid())
                        return reject("Malformed Cartesian inequality.");
                } else if (kind == "implicit") {
                    row.implicit = true;
                    if (!implicitEquationSource(row.expression.toStdString()).valid())
                        return reject("Malformed implicit equation.");
                } else if (kind == "parametric") {
                    if (!obj.value("yExpression").isString() ||
                        obj.value("yExpression").toString().isEmpty() ||
                        obj.value("yExpression").toString().size() > 2048 ||
                        !obj.value("tMin").isDouble() || !obj.value("tMax").isDouble())
                        return reject("Malformed parametric coordinate expression or interval.");
                    row.parametric = true;
                    row.yExpression = obj.value("yExpression").toString();
                    row.tMinimum = obj.value("tMin").toDouble();
                    row.tMaximum = obj.value("tMax").toDouble();
                    if (!validParameterInterval(row.tMinimum, row.tMaximum))
                        return reject("Invalid parametric parameter interval.");
                    const auto expressionBody = [](QString text, const QString& prefix) -> QString {
                        text = text.trimmed();
                        const int eq = text.indexOf('=');
                        if (eq < 0) return text;
                        QString label = text.left(eq).trimmed();
                        label.remove(' ');
                        return label.compare(prefix, Qt::CaseInsensitive) == 0
                            ? text.mid(eq + 1).trimmed() : QString();
                    };
                    const auto x = expressionBody(row.expression, "x(t)");
                    const auto y = expressionBody(row.yExpression, "y(t)");
                    if (x.isEmpty() || y.isEmpty())
                        return reject("Parametric coordinate labels must be x(t) and y(t).");
                    // Validate syntax against restored variable/function owners below,
                    // after the definition arrays are loaded in dependency order.
                } else if (kind == "polar") {
                    if (!obj.value("tMin").isDouble() || !obj.value("tMax").isDouble())
                        return reject("Malformed polar angular interval.");
                    row.polar = true;
                    row.tMinimum = obj.value("tMin").toDouble();
                    row.tMaximum = obj.value("tMax").toDouble();
                    if (!validParameterInterval(row.tMinimum, row.tMaximum) ||
                        !polarCoordinateSources(row.expression.toStdString()).valid())
                        return reject("Invalid polar radius or angular interval.");
                } else if (kind != "cartesian") {
                    return reject("Unknown curve representation kind.");
                }
            }
            stage.curves.push_back(std::move(row));
        }
        if (version >= 2) {
            for (const auto& key : {"sliders", "variables", "functions"}) {
                if (!root.value(key).isArray()) return reject(QString("Missing or invalid %1 array.").arg(key));
                if (root.value(key).toArray().size() > 128)
                    return reject(QString("Too many %1 entries.").arg(key));
            }
            std::set<std::string> seen;
            for (const auto& item : root.value("sliders").toArray()) {
                if (!item.isObject()) return reject("Invalid slider entry.");
                const auto obj = item.toObject();
                for (const auto& key : {"name", "value", "minimum", "maximum", "step"}) {
                    if (!obj.contains(key)) return reject(QString("Slider missing %1.").arg(key));
                }
                if (!obj.value("name").isString() || !obj.value("value").isDouble() ||
                    !obj.value("minimum").isDouble() || !obj.value("maximum").isDouble() ||
                    !obj.value("step").isDouble()) return reject("Invalid slider field type.");
                const WorkspaceSlider slider{obj.value("name").toString().toStdString(),
                    obj.value("value").toDouble(), obj.value("minimum").toDouble(),
                    obj.value("maximum").toDouble(), obj.value("step").toDouble()};
                VariableScope only;
                std::string detail;
                if (!only.declare(slider.name, &detail))
                    return reject("Invalid slider name: " + QString::fromStdString(detail));
                const auto canonical = *only.names().begin();
                if (slider.name != canonical || !seen.insert(canonical).second)
                    return reject("Duplicate or noncanonical slider name.");
                auto scale = VariableSliderScale::create(slider.minimum, slider.maximum, slider.step, &detail);
                if (!scale || !std::isfinite(slider.value) || slider.value < slider.minimum ||
                    slider.value > slider.maximum || slider.minimum < -1e9 ||
                    slider.maximum > 1e9 || slider.step < 1e-9 ||
                    (slider.value != 0 && std::abs(slider.value) < 1e-9))
                    return reject("Invalid slider range/value: " + QString::fromStdString(detail));
                const auto snapped = scale->valueAt(scale->nearestTick(slider.value));
                if (std::abs(snapped - slider.value) > 1e-8 * std::max(1., std::abs(slider.value)))
                    return reject("Slider value is not aligned to the saved step.");
                const auto value = stage.validatedVariables.define(canonical,
                    QString::number(snapped, 'g', std::numeric_limits<double>::max_digits10).toStdString());
                if (!value.accepted) return reject("Invalid slider assignment.");
                stage.sliders.push_back(slider);
            }
            for (const auto& item : root.value("variables").toArray()) {
                if (!item.isObject()) return reject("Invalid variable definition entry.");
                const auto obj = item.toObject();
                if (!obj.value("name").isString() || !obj.value("expression").isString())
                    return reject("Invalid variable name or expression.");
                const WorkspaceAssignment variable{obj.value("name").toString().toStdString(),
                                                   obj.value("expression").toString().toStdString()};
                VariableScope canonicalScope;
                std::string nameError;
                if (!canonicalScope.declare(variable.name, &nameError) ||
                    variable.name != *canonicalScope.names().begin() ||
                    variable.expression.empty() || variable.expression.size() > 2048 ||
                    !seen.insert(variable.name).second) return reject("Duplicate/invalid variable definition.");
                const auto result = stage.validatedVariables.define(
                    variable.name, MathExpressionInput::normalize(variable.expression));
                if (!result.accepted) return reject("Variable '" + QString::fromStdString(variable.name) +
                    "': " + QString::fromStdString(result.error));
                stage.assignments.push_back(variable);
            }
            stage.validatedFunctions = FunctionDefinitions(stage.validatedVariables.scope());
            std::set<std::string> seenFunctions;
            for (const auto& item : root.value("functions").toArray()) {
                if (!item.isObject()) return reject("Invalid function definition entry.");
                const auto obj = item.toObject();
                if (!obj.value("name").isString() || !obj.value("parameters").isArray() ||
                    !obj.value("body").isString()) return reject("Invalid function definition fields.");
                const WorkspaceFunction function{obj.value("name").toString().toStdString(),
                    obj.value("body").toString().toStdString(), {}};
                VariableScope functionName;
                std::string nameError;
                if (!functionName.declare(function.name, &nameError) ||
                    function.name != *functionName.names().begin() ||
                    function.body.empty() || function.body.size() > 2048 ||
                    !seenFunctions.insert(function.name).second)
                    return reject("Duplicate/invalid function body.");
                auto entry = function;
                for (const auto& parameter : obj.value("parameters").toArray()) {
                    if (!parameter.isString()) return reject("Invalid function parameter.");
                    entry.parameters.push_back(parameter.toString().toStdString());
                }
                const auto result = stage.validatedFunctions.define(
                    entry.name, entry.parameters, MathExpressionInput::normalize(entry.body));
                if (!result.accepted) return reject("Function '" + QString::fromStdString(entry.name) +
                    "': " + QString::fromStdString(result.error));
                stage.functions.push_back(std::move(entry));
            }
        }
        // Ensure parametric rows have valid coordinate expressions in the
        // reconstructed variable/function scope. Invalid files must not
        // mutate the live workspace state.
        if (version == 3) {
            VariableScope parameterScope = stage.validatedVariables.scope();
            if (!parameterScope.contains("t")) (void)parameterScope.declare("t");
            for (const auto& row : stage.curves) {
                if (row.inequality) {
                    const auto parsed = parseInequality(row.expression.toStdString());
                    if (!parsed.valid()) return reject("Invalid inequality comparator.");
                    Expression compiled(MathExpressionInput::normalize(parsed.expression),
                        stage.validatedVariables.scope(), stage.validatedFunctions.scope());
                    if (!compiled.valid()) return reject("Invalid inequality boundary: " +
                        QString::fromStdString(compiled.error()));
                    continue;
                }
                if (row.implicit) {
                    const auto source = implicitEquationSource(row.expression.toStdString());
                    VariableScope local = stage.validatedVariables.scope();
                    if (!local.contains("y")) (void)local.declare("y");
                    Expression compiled(MathExpressionInput::normalize(source.expression), local,
                                        stage.validatedFunctions.scope());
                    if (!compiled.valid()) return reject("Invalid implicit equation: " +
                                        QString::fromStdString(compiled.error()));
                    continue;
                }
                if (row.polar) {
                    const auto sources = polarCoordinateSources(row.expression.toStdString());
                    if (!sources.valid()) return reject("Invalid polar radius label or body.");
                    for (const auto& coordinate : {sources.x, sources.y}) {
                        Expression compiled(coordinate, parameterScope, stage.validatedFunctions.scope());
                        if (!compiled.valid() || compiled.dependsOnX())
                            return reject("Invalid polar radius: " +
                                QString::fromStdString(compiled.error()));
                    }
                    continue;
                }
                if (!row.parametric) continue;
                const auto body = [](QString text) {
                    const int equal = text.indexOf('=');
                    return equal < 0 ? text.trimmed() : text.mid(equal + 1).trimmed();
                };
                for (const auto& coordinate : {body(row.expression), body(row.yExpression)}) {
                    Expression compiled(MathExpressionInput::normalize(coordinate.toStdString()),
                        parameterScope, stage.validatedFunctions.scope());
                    if (!compiled.valid())
                        return reject("Invalid parametric expression: " +
                            QString::fromStdString(compiled.error()));
                    if (compiled.dependsOnX())
                        return reject("Parametric coordinates must use t, not Cartesian x.");
                }
            }
        }
        if (error) error->clear();
        output = std::move(stage);
        return true;
    }
};
} // namespace rationalstudio
