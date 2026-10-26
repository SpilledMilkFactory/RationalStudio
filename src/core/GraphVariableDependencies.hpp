#pragma once

#include "Expression.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace rationalstudio {

// Dependency names from VariableDefinitions::changed and compiled Expression
// introspection are canonical lowercase. Direct references and scalar names
// captured through nested user-defined functions must both be considered.
// An empty change set never invalidates anything; callers update bindings
// independently so unaffected jobs can keep their existing snapshot.
[[nodiscard]] inline bool graphCurveDependsOnAny(
    const Expression& expression, const std::vector<std::string>& changed) {
    if (!expression.valid() || changed.empty()) return false;
    const auto matches = [&changed](const std::vector<std::string>& deps) {
        for (const auto& dependency : deps) {
            if (std::find(changed.begin(), changed.end(), dependency) != changed.end())
                return true;
        }
        return false;
    };
    return matches(expression.referencedVariables()) ||
           matches(expression.functionVariableDependencies());
}

} // namespace rationalstudio
