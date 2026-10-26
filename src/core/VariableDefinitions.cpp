#include "VariableDefinitions.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <set>
#include <utility>

namespace rationalstudio {
namespace {

std::string_view trim(std::string_view text) {
    const auto white = [](char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    };
    while (!text.empty() && white(text.front())) text.remove_prefix(1);
    while (!text.empty() && white(text.back())) text.remove_suffix(1);
    return text;
}

// Reuse Issue #27's identifier validation and normalization, avoiding a
// second independent set of reserved-word or case-folding rules.
bool canonicalName(std::string_view input, std::string& name,
                   std::string* error = nullptr) {
    VariableScope probe;
    if (!probe.declare(trim(input), error)) return false;
    name = *probe.names().begin();
    return true;
}

VariableUpdate failure(std::string message) {
    VariableUpdate result;
    result.error = std::move(message);
    return result;
}

bool references(const std::vector<std::string>& deps, const std::string& key) {
    return std::binary_search(deps.begin(), deps.end(), key);
}

} // namespace

VariableDefinitions::VariableDefinitions(AngleMode angle) noexcept
    : angle_(angle) {}

VariableUpdate VariableDefinitions::assign(std::string_view statement) {
    const auto equal = statement.find('=');
    if (equal == std::string_view::npos ||
        statement.find('=', equal + 1) != std::string_view::npos) {
        return failure("Expected one assignment of the form name = expression");
    }
    return define(trim(statement.substr(0, equal)),
                  trim(statement.substr(equal + 1)));
}

VariableUpdate VariableDefinitions::define(std::string_view name,
                                           std::string_view expression) {
    std::string key, error;
    if (!canonicalName(name, key, &error)) return failure(error);
    expression = trim(expression);
    if (expression.empty()) return failure("Assignment expression is empty");

    VariableScope available = scope();
    if (!available.contains(key)) {
        if (!available.declare(key, &error)) return failure(error);
    }
    const auto compiled = std::make_shared<Expression>(
        std::string(expression), available);
    if (!compiled->valid()) return failure(compiled->error());
    if (compiled->dependsOnX() || compiled->dependsOnAns()) {
        return failure("Scalar assignments cannot depend on x or ans");
    }
    const auto& deps = compiled->referencedVariables();
    for (const auto& dep : deps) {
        if (dep != key && !contains(dep)) {
            return failure("Undefined variable '" + dep + "'");
        }
    }
    if (wouldCreateCycle(key, deps, error)) return failure(error);

    // The entire candidate has been validated before modifying entries_;
    // rejected edits leave existing dependency edges and values unchanged.
    auto& slot = entries_[key];
    slot.source = std::string(expression);
    slot.expression = compiled;
    slot.dependencies = deps;
    return refresh({key});
}

bool VariableDefinitions::wouldCreateCycle(
    const std::string& key, const std::vector<std::string>& proposed,
    std::string& error) const {
    // Only the named definition's outgoing edges change. Consequently every
    // newly introduced cycle must lead from one proposed dependency to key.
    std::set<std::string> visited;
    std::function<bool(const std::string&)> reachesTarget =
        [&](const std::string& name) -> bool {
            if (name == key) return true;
            if (!visited.insert(name).second) return false;
            const auto it = entries_.find(name);
            if (it == entries_.end()) return false;
            for (const auto& dep : it->second.dependencies) {
                if (reachesTarget(dep)) return true;
            }
            return false;
        };
    for (const auto& dep : proposed) {
        visited.clear();
        if (reachesTarget(dep)) {
            error = "Circular dependency involving '" + key + "' and '" + dep + "'";
            return true;
        }
    }
    return false;
}

std::vector<std::string> VariableDefinitions::affectedBy(
    const std::string& name) const {
    std::set<std::string> affected{name};
    bool grew;
    do {
        grew = false;
        for (const auto& [other, entry] : entries_) {
            if (affected.contains(other)) continue;
            for (const auto& dep : entry.dependencies) {
                if (affected.contains(dep)) {
                    affected.insert(other);
                    grew = true;
                    break;
                }
            }
        }
    } while (grew);
    return {affected.begin(), affected.end()};
}

VariableUpdate VariableDefinitions::refresh(const std::vector<std::string>& roots) {
    std::set<std::string> affected;
    for (const auto& root : roots) {
        for (const auto& name : affectedBy(root)) affected.insert(name);
    }

    std::set<std::string> visited;
    std::vector<std::string> order;
    std::function<void(const std::string&)> visit = [&](const std::string& key) {
        if (!visited.insert(key).second) return;
        const auto it = entries_.find(key);
        if (it == entries_.end()) return;
        for (const auto& dep : it->second.dependencies) {
            if (affected.contains(dep)) visit(dep);
        }
        order.push_back(key);
    };
    for (const auto& name : affected) visit(name);

    VariableBindings active = bindings();
    VariableUpdate result;
    result.accepted = true;
    for (const auto& name : order) {
        auto& item = entries_.at(name);
        const double computed = item.expression->evaluate(0, 0, angle_, active);
        const std::optional<double> next = std::isfinite(computed)
            ? std::optional<double>{computed} : std::nullopt;
        if (item.revision == 0 || item.value != next) {
            item.revision = ++nextRevision_;
            result.changed.push_back(name);
        }
        item.value = next;
        if (next) active.set(name, *next);
        else active.erase(name);
        result.recomputed.push_back(name);
    }
    return result;
}

VariableUpdate VariableDefinitions::erase(std::string_view name) {
    std::string key, error;
    if (!canonicalName(name, key, &error)) return failure(error);
    if (!contains(key)) return failure("Unknown variable '" + key + "'");
    for (const auto& [other, entry] : entries_) {
        if (other != key && references(entry.dependencies, key)) {
            return failure("Cannot remove '" + key + "': referenced by '" + other + "'");
        }
    }
    entries_.erase(key);
    VariableUpdate result;
    result.accepted = true;
    result.changed = {key};
    return result;
}

VariableUpdate VariableDefinitions::setAngleMode(AngleMode angle) {
    if (angle == angle_) {
        VariableUpdate result;
        result.accepted = true;
        return result;
    }
    angle_ = angle;
    std::vector<std::string> roots = names();
    return refresh(roots);
}

bool VariableDefinitions::contains(std::string_view name) const {
    std::string key;
    return canonicalName(name, key) && entries_.contains(key);
}

std::optional<double> VariableDefinitions::value(std::string_view name) const {
    std::string key;
    if (!canonicalName(name, key)) return std::nullopt;
    const auto it = entries_.find(key);
    return it == entries_.end() ? std::nullopt : it->second.value;
}

std::string VariableDefinitions::source(std::string_view name) const {
    std::string key;
    if (!canonicalName(name, key)) return {};
    const auto it = entries_.find(key);
    return it == entries_.end() ? std::string{} : it->second.source;
}

std::vector<std::string> VariableDefinitions::directDependencies(
    std::string_view name) const {
    std::string key;
    if (!canonicalName(name, key)) return {};
    const auto it = entries_.find(key);
    return it == entries_.end() ? std::vector<std::string>{} : it->second.dependencies;
}

std::vector<std::string> VariableDefinitions::dependents(
    std::string_view name) const {
    std::string key;
    if (!canonicalName(name, key) || !contains(key)) return {};
    auto all = affectedBy(key);
    all.erase(std::remove(all.begin(), all.end(), key), all.end());
    return all;
}

std::uint64_t VariableDefinitions::revision(std::string_view name) const {
    std::string key;
    if (!canonicalName(name, key)) return 0;
    const auto it = entries_.find(key);
    return it == entries_.end() ? 0 : it->second.revision;
}

std::vector<std::string> VariableDefinitions::names() const {
    std::vector<std::string> result;
    result.reserve(entries_.size());
    for (const auto& [name, entry] : entries_) {
        (void)entry;
        result.push_back(name);
    }
    return result;
}

VariableScope VariableDefinitions::scope() const {
    VariableScope result;
    for (const auto& [name, entry] : entries_) {
        (void)entry;
        result.declare(name);
    }
    return result;
}

VariableBindings VariableDefinitions::bindings() const {
    VariableBindings result;
    for (const auto& [name, entry] : entries_) {
        if (entry.value) result.set(name, *entry.value);
    }
    return result;
}

double VariableDefinitions::evaluate(const Expression& expression,
                                     double x, double ans) const noexcept {
    // Bindings are snapshots; no pointer to a local binding object is retained.
    return expression.evaluate(x, ans, angle_, bindings());
}

} // namespace rationalstudio
