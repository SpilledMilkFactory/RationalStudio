#pragma once

#include "Expression.hpp"

#include <cstddef>
#include <functional>
#include <limits>
#include <vector>

namespace rationalstudio {

// M4-03 / Issue #48. A finite, bracket-preserving scalar root solver.
// Residual acceptance is numeric evidence, not a proof of continuity or
// existence. Only sign-changing intervals and sampled zeros are discoverable.
using RootFunction = std::function<double(double)>;

enum class RootMethod { SafeguardedSecantBisection };
enum class RootStatus {
    Converged,
    InvalidInput,
    UndefinedEndpoint,
    NoSignChange,
    UndefinedDuringSearch,
    EvaluationLimit,
    IterationLimit,
    Unresolved
};

struct RootOptions {
    double xAbsoluteTolerance = 1e-12;
    double xRelativeTolerance = 1e-12;
    double residualTolerance = 1e-10;
    std::size_t maxIterations = 128;
    std::size_t maxEvaluations = 512;
};

struct RootResult {
    RootStatus status = RootStatus::InvalidInput;
    RootMethod method = RootMethod::SafeguardedSecantBisection;
    double x = std::numeric_limits<double>::quiet_NaN();
    double residual = std::numeric_limits<double>::infinity();
    double bracketLower = std::numeric_limits<double>::quiet_NaN();
    double bracketUpper = std::numeric_limits<double>::quiet_NaN();
    std::size_t evaluations = 0;
    std::size_t iterations = 0;
    [[nodiscard]] bool converged() const noexcept {
        return status == RootStatus::Converged;
    }
};

// Search is a finite uniform scan followed by the same bracketed solver.
// It can miss even-multiplicity off-grid roots and unsampled oscillations.
enum class RootSearchStatus {
    Complete, InvalidInput, IncompleteDomain, EvaluationLimit, ResultLimit
};

struct RootSearchOptions {
    RootOptions root{};
    std::size_t subdivisions = 512;
    std::size_t maxEvaluations = 65536; // TOTAL across scan and all refinements
    std::size_t maxResults = 64;
};

struct RootSearchResult {
    RootSearchStatus status = RootSearchStatus::InvalidInput;
    std::vector<RootResult> roots;
    std::size_t evaluations = 0;
    std::size_t invalidSamples = 0;
    std::size_t rejectedBrackets = 0;
    [[nodiscard]] bool complete() const noexcept {
        return status == RootSearchStatus::Complete;
    }
};

[[nodiscard]] RootResult solveBracketedRoot(
    const RootFunction& function, double lower, double upper,
    const RootOptions& options = {});

[[nodiscard]] RootSearchResult findRootsInDomain(
    const RootFunction& function, double lower, double upper,
    const RootSearchOptions& options = {});

// Compiled-expression overloads take snapshots of the supplied scalar values.
// The equation overload solves left(x) - right(x) = 0, not a parser-level '='.
[[nodiscard]] RootResult solveBracketedRoot(
    const Expression& expression, double lower, double upper, AngleMode angle,
    const VariableBindings& bindings, double ans = 0.0,
    const RootOptions& options = {});

[[nodiscard]] RootResult solveEquation(
    const Expression& left, const Expression& right,
    double lower, double upper, AngleMode angle,
    const VariableBindings& bindings, double ans = 0.0,
    const RootOptions& options = {});

[[nodiscard]] RootSearchResult findRootsInDomain(
    const Expression& expression, double lower, double upper, AngleMode angle,
    const VariableBindings& bindings, double ans = 0.0,
    const RootSearchOptions& options = {});

[[nodiscard]] const char* rootStatusName(RootStatus status) noexcept;
[[nodiscard]] const char* rootSearchStatusName(RootSearchStatus status) noexcept;
[[nodiscard]] const char* rootMethodName(RootMethod method) noexcept;

} // namespace rationalstudio
