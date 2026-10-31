#include "DescriptiveStatistics.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace rationalstudio {
namespace {
constexpr double kNan = std::numeric_limits<double>::quiet_NaN();

StatisticsStatus validate(std::span<const double> values) noexcept {
    if (values.empty()) return StatisticsStatus::EmptyInput;
    if (values.size() > kMaxStatisticsSamples) return StatisticsStatus::TooManySamples;
    for (double value : values)
        if (!std::isfinite(value)) return StatisticsStatus::NonFiniteInput;
    return StatisticsStatus::Success;
}

bool publish(long double value, double& output) noexcept {
    if (!std::isfinite(value) ||
        std::abs(value) > static_cast<long double>(std::numeric_limits<double>::max()))
        return false;
    output = static_cast<double>(value);
    return std::isfinite(output);
}

long double interpolate(const std::vector<double>& sorted, long double rank) {
    const std::size_t low = static_cast<std::size_t>(std::floor(rank));
    const std::size_t high = std::min(low + 1, sorted.size() - 1);
    const long double fraction = rank - static_cast<long double>(low);
    const long double a = sorted[low], b = sorted[high];
    return a + (b - a) * fraction;
}
}

const char* statisticsStatusName(StatisticsStatus status) noexcept {
    switch (status) {
    case StatisticsStatus::Success: return "success";
    case StatisticsStatus::EmptyInput: return "empty input";
    case StatisticsStatus::NonFiniteInput: return "non-finite input";
    case StatisticsStatus::TooManySamples: return "sample limit exceeded";
    case StatisticsStatus::InvalidPercentile: return "invalid percentile";
    case StatisticsStatus::NonFiniteResult: return "non-finite result";
    }
    return "unknown";
}

DescriptiveStatisticsResult summarizeStatistics(std::span<const double> values) {
    DescriptiveStatisticsResult out;
    out.status = validate(values);
    if (out.status != StatisticsStatus::Success) return out;

    long double mean = 0.0L;
    long double m2 = 0.0L;
    long double sum = 0.0L;
    long double compensation = 0.0L;
    std::size_t count = 0;
    double minimum = values.front(), maximum = values.front();
    for (double number : values) {
        const long double x = static_cast<long double>(number);
        ++count;
        const long double delta = x - mean;
        mean += delta / static_cast<long double>(count);
        const long double delta2 = x - mean;
        m2 += delta * delta2;
        // Neumaier compensated sum in extended precision.
        const long double next = sum + x;
        if (std::abs(sum) >= std::abs(x)) compensation += (sum - next) + x;
        else compensation += (x - next) + sum;
        sum = next;
        minimum = std::min(minimum, number);
        maximum = std::max(maximum, number);
    }
    if (m2 < 0.0L && m2 > -1e-24L) m2 = 0.0L; // defensive roundoff
    if (!(m2 >= 0.0L) || !std::isfinite(m2)) {
        out.status = StatisticsStatus::NonFiniteResult;
        return out;
    }
    const long double popVariance = m2 / static_cast<long double>(count);
    const long double sampleVariance = count > 1
        ? m2 / static_cast<long double>(count - 1) : 0.0L;
    std::vector<double> sorted(values.begin(), values.end());
    std::sort(sorted.begin(), sorted.end());
    const long double middle = interpolate(sorted, static_cast<long double>(count - 1) / 2.0L);

    double total = kNan, avg = kNan, spread = kNan, median = kNan;
    double popVar = kNan, popStd = kNan, sampleVar = kNan, sampleStd = kNan;
    const bool ok = publish(sum + compensation, total) &&
        publish(mean, avg) &&
        publish(static_cast<long double>(maximum) - minimum, spread) &&
        publish(middle, median) &&
        publish(popVariance, popVar) &&
        publish(std::sqrt(popVariance), popStd) &&
        (count == 1 || (publish(sampleVariance, sampleVar) &&
                        publish(std::sqrt(sampleVariance), sampleStd)));
    if (!ok) {
        out.status = StatisticsStatus::NonFiniteResult;
        return out;
    }
    out.count = count;
    out.sum = total;
    out.mean = avg;
    out.minimum = minimum;
    out.maximum = maximum;
    out.range = spread;
    out.median = median;
    out.populationVariance = popVar;
    out.populationStdDev = popStd;
    out.sampleVariance = sampleVar;
    out.sampleStdDev = sampleStd;
    return out;
}

PercentileResult calculatePercentile(std::span<const double> values, double percentile) {
    PercentileResult out;
    if (!std::isfinite(percentile) || percentile < 0.0 || percentile > 100.0) {
        out.status = StatisticsStatus::InvalidPercentile;
        return out;
    }
    out.status = validate(values);
    if (out.status != StatisticsStatus::Success) return out;
    std::vector<double> sorted(values.begin(), values.end());
    std::sort(sorted.begin(), sorted.end());
    const long double rank = (static_cast<long double>(sorted.size()) - 1.0L) *
        static_cast<long double>(percentile) / 100.0L;
    if (!publish(interpolate(sorted, rank), out.value))
        out.status = StatisticsStatus::NonFiniteResult;
    return out;
}
} // namespace rationalstudio
