#pragma once

#include <cstddef>
#include <limits>
#include <span>
#include <vector>

namespace rationalstudio {

// M4-06 / Issue #51. Qt-independent descriptive statistics for finite real data.
// All results are double precision; extended precision is used internally to
// mitigate (not eliminate) rounding. These are numerical, not exact statistics.
enum class StatisticsStatus {
    Success,
    EmptyInput,
    NonFiniteInput,
    TooManySamples,
    InvalidPercentile,
    NonFiniteResult
};

[[nodiscard]] const char* statisticsStatusName(StatisticsStatus status) noexcept;

// A finite resource bound for sorting/processing input datasets.
inline constexpr std::size_t kMaxStatisticsSamples = 1'000'000;

struct DescriptiveStatisticsResult {
    StatisticsStatus status = StatisticsStatus::EmptyInput;
    std::size_t count = 0;
    double sum = std::numeric_limits<double>::quiet_NaN();
    double mean = std::numeric_limits<double>::quiet_NaN();
    double minimum = std::numeric_limits<double>::quiet_NaN();
    double maximum = std::numeric_limits<double>::quiet_NaN();
    double range = std::numeric_limits<double>::quiet_NaN();
    double median = std::numeric_limits<double>::quiet_NaN();
    double populationVariance = std::numeric_limits<double>::quiet_NaN();
    double populationStdDev = std::numeric_limits<double>::quiet_NaN();
    // Undefined for count < 2, even if population moments are available.
    double sampleVariance = std::numeric_limits<double>::quiet_NaN();
    double sampleStdDev = std::numeric_limits<double>::quiet_NaN();
    [[nodiscard]] bool successful() const noexcept { return status == StatisticsStatus::Success; }
    [[nodiscard]] bool hasSampleVariance() const noexcept { return successful() && count >= 2; }
};

struct PercentileResult {
    StatisticsStatus status = StatisticsStatus::EmptyInput;
    double value = std::numeric_limits<double>::quiet_NaN();
    [[nodiscard]] bool successful() const noexcept { return status == StatisticsStatus::Success; }
};

// Computes population variance M2/n and sample variance M2/(n-1).
// Uses long-double Welford moments. Input must be nonempty and finite.
// No outputs are published if an essential double-valued result overflows.
[[nodiscard]] DescriptiveStatisticsResult summarizeStatistics(std::span<const double> values);

// Uses sorted type-7 (linear interpolation) quantiles: rank = (n-1)*p/100.
// The percentile must be finite and in [0,100]. Endpoints select min/max.
[[nodiscard]] PercentileResult calculatePercentile(std::span<const double> values,
                                                   double percentile);

} // namespace rationalstudio
