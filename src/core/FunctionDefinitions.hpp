#pragma once

#include "Expression.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace rationalstudio {

struct FunctionUpdate {
    bool accepted = false;
    std::string error;
    // Includes the edited function and callers that were recompiled.
    std::vector<std::string> recompiled;
};

// A Qt-independent owner for compiled user-defined functions. Definitions
// reference only previously defined functions (or existing ones on edit),
// use lexical parameter bindings, and may capture named scalar values supplied
// by a VariableBindings snapshot at evaluation time.
//
// Definitions are transactionally rebuilt and published as immutable function
// snapshots; an Expression compiled against an old scope remains valid and
// retains its previous semantics. Recompile a consumer against scope() to
// observe a changed function definition (Issue #31 will integrate this with
// graph invalidation). GUI rows, sliders, and persistence are out of scope.
class FunctionDefinitions {
public:
    explicit FunctionDefinitions(VariableScope variables = {});

    FunctionUpdate define(std::string_view statement); // f(x) = x*x
    FunctionUpdate define(std::string_view name,
                          const std::vector<std::string>& parameters,
                          std::string_view body);
    FunctionUpdate erase(std::string_view name);
    FunctionUpdate setVariableScope(const VariableScope& variables);

    [[nodiscard]] bool contains(std::string_view name) const;
    [[nodiscard]] FunctionScope scope() const { return published_; }
    [[nodiscard]] std::vector<std::string> names() const;
    [[nodiscard]] std::vector<std::string> parameters(std::string_view name) const;
    [[nodiscard]] std::vector<std::string> directFunctionDependencies(std::string_view name) const;
    [[nodiscard]] std::vector<std::string> variableDependencies(std::string_view name) const;
    [[nodiscard]] std::string source(std::string_view name) const;

private:
    struct Entry {
        std::vector<std::string> parameters;
        std::string source;
        std::vector<std::string> callees;
        std::vector<std::string> variables;
    };
    std::map<std::string, Entry, std::less<>> entries_;
    VariableScope variables_;
    FunctionScope published_;

    FunctionUpdate rebuild(const std::map<std::string, Entry, std::less<>>& candidate,
                           const VariableScope& variables);
};

} // namespace rationalstudio
