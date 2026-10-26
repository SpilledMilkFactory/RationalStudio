#pragma once

#include "Expression.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rationalstudio {

// Every successful edit is committed atomically. `recomputed` includes the
// edited name and its transitive dependents, in dependency-first order;
// `changed` lists only names whose numeric/undefined result changed.
// Clients can use `changed` to avoid invalidating unrelated graph geometry.
struct VariableUpdate {
    bool accepted = false;
    std::string error;
    std::vector<std::string> recomputed;
    std::vector<std::string> changed;
};

// An owner of named scalar assignments. Forward references are rejected:
// create the dependency first, then refer to it. Cyclic redefinitions,
// malformed expressions, x and ans dependencies do not mutate any state.
// Evaluation-domain errors (e.g. sqrt(-1)) yield an undefined value and
// propagate to dependents until a later assignment makes them defined.
//
// This is a Qt-independent core API, not the GUI assignment editor, graph
// invalidation system, function-definition language, or workspace persistence.
class VariableDefinitions {
public:
    explicit VariableDefinitions(AngleMode angle = AngleMode::Radians) noexcept;

    VariableUpdate assign(std::string_view statement);
    VariableUpdate define(std::string_view name, std::string_view expression);
    VariableUpdate erase(std::string_view name);
    VariableUpdate setAngleMode(AngleMode angle);

    [[nodiscard]] bool contains(std::string_view name) const;
    [[nodiscard]] std::optional<double> value(std::string_view name) const;
    [[nodiscard]] std::string source(std::string_view name) const;
    [[nodiscard]] std::vector<std::string> directDependencies(std::string_view name) const;
    [[nodiscard]] std::vector<std::string> dependents(std::string_view name) const;
    [[nodiscard]] std::uint64_t revision(std::string_view name) const;
    [[nodiscard]] std::vector<std::string> names() const;
    [[nodiscard]] VariableScope scope() const;
    [[nodiscard]] VariableBindings bindings() const;
    [[nodiscard]] AngleMode angleMode() const noexcept { return angle_; }

    // Evaluates an already-compiled external expression with the current
    // bindings. Graph expressions may depend on x; variable definitions may not.
    [[nodiscard]] double evaluate(const Expression& expression,
                                  double x = 0, double ans = 0) const noexcept;

private:
    struct Entry {
        std::string source;
        std::shared_ptr<const Expression> expression;
        std::vector<std::string> dependencies;
        std::optional<double> value;
        std::uint64_t revision = 0;
    };

    std::map<std::string, Entry, std::less<>> entries_;
    AngleMode angle_;
    std::uint64_t nextRevision_ = 0;

    [[nodiscard]] std::vector<std::string> affectedBy(const std::string& name) const;
    [[nodiscard]] bool wouldCreateCycle(const std::string& name,
                                        const std::vector<std::string>& proposed,
                                        std::string& error) const;
    VariableUpdate refresh(const std::vector<std::string>& roots);
};

} // namespace rationalstudio
