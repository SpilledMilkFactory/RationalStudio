#include "FunctionDefinitions.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <set>
#include <utility>

namespace rationalstudio {
namespace {

std::string_view trim(std::string_view text) {
    const auto white = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!text.empty() && white(text.front())) text.remove_prefix(1);
    while (!text.empty() && white(text.back())) text.remove_suffix(1);
    return text;
}

bool canonical(std::string_view input, std::string& result, std::string* error = nullptr) {
    VariableScope scope;
    if (!scope.declare(trim(input), error)) return false;
    result = *scope.names().begin();
    return true;
}

bool parameterName(std::string_view input, std::string& result,
                   std::string* error = nullptr) {
    input = trim(input);
    if (input == "x" || input == "X") {
        result = "x";
        if (error) error->clear();
        return true;
    }
    return canonical(input, result, error);
}

FunctionUpdate rejected(std::string error) {
    FunctionUpdate result;
    result.error = std::move(error);
    return result;
}

} // namespace

FunctionDefinitions::FunctionDefinitions(VariableScope variables)
    : variables_(std::move(variables)) {}

FunctionUpdate FunctionDefinitions::define(std::string_view statement) {
    const auto equal = statement.find('=');
    if (equal == std::string_view::npos ||
        statement.find('=', equal + 1) != std::string_view::npos) {
        return rejected("Expected function definition name(arg) = expression");
    }
    auto left = trim(statement.substr(0, equal));
    const auto open = left.find('(');
    const auto close = left.rfind(')');
    if (open == std::string_view::npos || close == std::string_view::npos ||
        close != left.size() - 1 || close < open ||
        left.find('(', open + 1) != std::string_view::npos) {
        return rejected("Expected function signature name(arg) before '='");
    }
    auto argumentList = trim(left.substr(open + 1, close - open - 1));
    if (argumentList.empty()) return rejected("Functions need at least one parameter");

    std::vector<std::string> arguments;
    while (true) {
        const auto comma = argumentList.find(',');
        arguments.emplace_back(trim(argumentList.substr(0, comma)));
        if (comma == std::string_view::npos) break;
        argumentList.remove_prefix(comma + 1);
    }
    return define(trim(left.substr(0, open)), arguments,
                  trim(statement.substr(equal + 1)));
}

FunctionUpdate FunctionDefinitions::define(
    std::string_view name, const std::vector<std::string>& parameters,
    std::string_view body) {
    std::string key, error;
    if (!canonical(name, key, &error)) return rejected(error);
    if (variables_.contains(key)) return rejected("Function name conflicts with scalar '" + key + "'");
    if (parameters.empty() || parameters.size() > 8) {
        return rejected("Functions require between 1 and 8 parameters");
    }
    std::vector<std::string> normalized;
    std::set<std::string> unique;
    for (const auto& parameter : parameters) {
        std::string local;
        if (!parameterName(parameter, local, &error)) return rejected(error);
        if (!unique.insert(local).second) return rejected("Duplicate parameter '" + local + "'");
        if (local == key || published_.contains(local)) {
            return rejected("Parameter conflicts with a function name '" + local + "'");
        }
        normalized.push_back(std::move(local));
    }
    body = trim(body);
    if (body.empty()) return rejected("Function body is empty");

    auto candidate = entries_;
    // Validate against the currently committed definitions first. This gives
    // explicit errors for forward references, malformed expressions and arity.
    VariableScope locals = variables_;
    for (const auto& parameter : normalized) {
        if (parameter != "x" && !locals.contains(parameter)) locals.declare(parameter);
    }
    Expression probe(std::string(body), locals, published_);
    if (!probe.valid()) return rejected(probe.error());
    if (probe.dependsOnAns()) return rejected("Function bodies cannot reference ans");
    if (probe.dependsOnX() && !unique.contains("x")) {
        return rejected("Undeclared x is not a function parameter");
    }

    auto& slot = candidate[key];
    slot.parameters = std::move(normalized);
    slot.source = std::string(body);
    slot.callees = probe.referencedFunctions();
    return rebuild(candidate, variables_);
}

FunctionUpdate FunctionDefinitions::rebuild(
    const std::map<std::string, Entry, std::less<>>& candidate,
    const VariableScope& variables) {
    // Find a topological order and reject cycles BEFORE publishing any edits.
    std::map<std::string, int> state;
    std::vector<std::string> ordered;
    std::string error;
    std::function<bool(const std::string&)> visit = [&](const std::string& key) {
        if (state[key] == 1) {
            error = "Recursive function dependency involving '" + key + "'";
            return false;
        }
        if (state[key] == 2) return true;
        state[key] = 1;
        for (const auto& callee : candidate.at(key).callees) {
            if (!candidate.contains(callee)) {
                error = "Unknown function '" + callee + "'";
                return false;
            }
            if (!visit(callee)) return false;
        }
        state[key] = 2;
        ordered.push_back(key);
        return true;
    };
    for (const auto& [key, entry] : candidate) {
        (void)entry;
        if (!visit(key)) return rejected(error);
    }

    // Bound nested user-function calls to avoid overflowing the native C++
    // stack on very deep but acyclic dependency chains.
    std::map<std::string, std::size_t> callDepth;
    for (const auto& key : ordered) {
        std::size_t depth = 1;
        for (const auto& callee : candidate.at(key).callees) {
            depth = std::max(depth, callDepth.at(callee) + 1);
        }
        if (depth > 64) {
            return rejected("Function call dependency depth exceeds 64");
        }
        callDepth[key] = depth;
    }

    FunctionScope ready;
    auto updated = candidate;
    for (const auto& key : ordered) {
        auto& entry = updated.at(key);
        VariableScope locals = variables;
        for (const auto& parameter : entry.parameters) {
            if (parameter != "x" && !locals.contains(parameter)) locals.declare(parameter);
        }
        auto body = std::make_shared<Expression>(entry.source, locals, ready);
        if (!body->valid()) return rejected("Function '" + key + "': " + body->error());
        if (body->dependsOnAns()) return rejected("Function '" + key + "' cannot reference ans");
        if (body->dependsOnX() &&
            std::find(entry.parameters.begin(), entry.parameters.end(), "x") == entry.parameters.end()) {
            return rejected("Function '" + key + "' uses undeclared x");
        }
        entry.callees = body->referencedFunctions();
        std::set<std::string> captured(body->functionVariableDependencies().begin(),
                                       body->functionVariableDependencies().end());
        for (const auto& variable : body->referencedVariables()) {
            if (std::find(entry.parameters.begin(), entry.parameters.end(), variable) ==
                entry.parameters.end()) captured.insert(variable);
        }
        entry.variables.assign(captured.begin(), captured.end());
        auto callable = std::make_shared<UserFunction>();
        callable->name = key;
        callable->parameters = entry.parameters;
        callable->body = std::move(body);
        callable->variableDependencies = entry.variables;
        ready.entries_.emplace(key, std::move(callable));
    }
    FunctionUpdate result;
    result.accepted = true;
    for (const auto& key : ordered) {
        auto old = published_.lookup(key);
        auto fresh = ready.lookup(key);
        // Report changed bodies and functions that call them, not unrelated
        // definitions. The rebuilt function objects are safe snapshots.
        const auto previous = entries_.find(key);
        if (previous == entries_.end() ||
            previous->second.source != updated.at(key).source ||
            previous->second.parameters != updated.at(key).parameters ||
            std::any_of(updated.at(key).callees.begin(), updated.at(key).callees.end(),
                [&](const std::string& dep) {
                    return std::find(result.recompiled.begin(), result.recompiled.end(), dep) !=
                           result.recompiled.end();
                })) {
            result.recompiled.push_back(key);
        }
        (void)old;
        (void)fresh;
    }
    entries_ = std::move(updated);
    published_ = std::move(ready);
    variables_ = variables;
    return result;
}

FunctionUpdate FunctionDefinitions::erase(std::string_view name) {
    std::string key, error;
    if (!canonical(name, key, &error)) return rejected(error);
    if (!entries_.contains(key)) return rejected("Unknown function '" + key + "'");
    for (const auto& [other, entry] : entries_) {
        if (other != key && std::find(entry.callees.begin(), entry.callees.end(), key) !=
                               entry.callees.end()) {
            return rejected("Cannot remove '" + key + "': used by '" + other + "'");
        }
    }
    auto next = entries_;
    next.erase(key);
    return rebuild(next, variables_);
}

FunctionUpdate FunctionDefinitions::setVariableScope(const VariableScope& variables) {
    for (const auto& name : variables.names()) {
        if (entries_.contains(name)) return rejected("Variable name conflicts with function '" + name + "'");
    }
    return rebuild(entries_, variables);
}

bool FunctionDefinitions::contains(std::string_view name) const {
    return published_.contains(name);
}

std::vector<std::string> FunctionDefinitions::names() const {
    return published_.names();
}

std::vector<std::string> FunctionDefinitions::parameters(std::string_view name) const {
    const auto found = published_.lookup(name);
    return found ? found->parameters : std::vector<std::string>{};
}

std::vector<std::string> FunctionDefinitions::directFunctionDependencies(std::string_view name) const {
    std::string key;
    if (!canonical(name, key)) return {};
    const auto it = entries_.find(key);
    return it == entries_.end() ? std::vector<std::string>{} : it->second.callees;
}

std::vector<std::string> FunctionDefinitions::variableDependencies(std::string_view name) const {
    std::string key;
    if (!canonical(name, key)) return {};
    const auto it = entries_.find(key);
    return it == entries_.end() ? std::vector<std::string>{} : it->second.variables;
}

std::string FunctionDefinitions::source(std::string_view name) const {
    std::string key;
    if (!canonical(name, key)) return {};
    const auto it = entries_.find(key);
    return it == entries_.end() ? std::string{} : it->second.source;
}

} // namespace rationalstudio
