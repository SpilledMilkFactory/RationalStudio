#include "DescriptiveStatistics.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace rationalstudio;
namespace {
int checks = 0;
int failures = 0;
void check(bool yes, const std::string& label) {
    ++checks;
    if (!yes) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
bool near(double actual, long double expected, long double tol = 1e-10L) {
    return std::isfinite(actual) &&
        std::abs(static_cast<long double>(actual) - expected) <=
        tol * (1.0L + std::abs(expected));
}
}
int main() {
    const std::vector<double> example{2,4,4,4,5,5,7,9};
    const auto a = summarizeStatistics(example);
    check(a.successful(), "sample summary success");
    check(a.count == 8, "count");
    check(a.sum == 40.0, "sum");
    check(a.mean == 5.0, "mean");
    check(a.minimum == 2.0 && a.maximum == 9.0, "minimum maximum");
    check(a.range == 7.0, "range");
    check(a.median == 4.5, "even median");
    check(near(a.populationVariance, 4.0L), "population variance");
    check(near(a.populationStdDev, 2.0L), "population standard deviation");
    check(near(a.sampleVariance, 32.0L / 7.0L), "sample variance n-1");
    check(near(a.sampleStdDev, std::sqrt(32.0L / 7.0L)), "sample stddev");
    check(a.hasSampleVariance(), "sample available");

    const std::vector<double> singleton{17};
    const auto one = summarizeStatistics(singleton);
    check(one.successful() && one.mean == 17 && one.populationVariance == 0,
          "singleton valid population");
    check(!one.hasSampleVariance() && std::isnan(one.sampleVariance) &&
          std::isnan(one.sampleStdDev), "singleton sample undefined");
    check(one.median == 17 && one.range == 0, "singleton median and range");

    const auto negative = summarizeStatistics(std::vector<double>{-5,-3,-1});
    check(negative.successful() && negative.mean == -3 && negative.median == -3,
          "negative values");
    check(near(negative.populationVariance, 8.0L / 3.0L),
          "negative population variance");
    const auto constant = summarizeStatistics(std::vector<double>{-2,-2,-2,-2});
    check(constant.successful() && constant.populationVariance == 0 &&
          constant.sampleVariance == 0, "constant variance zero");

    const auto p0 = calculatePercentile(example, 0);
    const auto p25 = calculatePercentile(example, 25);
    const auto p50 = calculatePercentile(example, 50);
    const auto p75 = calculatePercentile(example, 75);
    const auto p100 = calculatePercentile(example, 100);
    check(p0.successful() && p0.value == 2, "percentile minimum");
    check(p25.successful() && p25.value == 4, "type7 lower quartile");
    check(p50.successful() && p50.value == 4.5, "type7 median");
    check(p75.successful() && p75.value == 5.5, "type7 upper quartile");
    check(p100.successful() && p100.value == 9, "percentile maximum");
    check(calculatePercentile(std::vector<double>{0, 10}, 12.5).value == 1.25,
          "fractional percentile interpolation");
    check(calculatePercentile(singleton, 99).value == 17, "singleton percentile");
    check(calculatePercentile(std::vector<double>{-10,10}, 50).value == 0,
          "no-overflow interpolated opposite signs");

    check(summarizeStatistics(std::vector<double>{}).status == StatisticsStatus::EmptyInput,
          "empty summary rejected");
    check(calculatePercentile({}, 50).status == StatisticsStatus::EmptyInput,
          "empty percentile rejected");
    check(summarizeStatistics(std::vector<double>{1,std::numeric_limits<double>::infinity()}).status ==
          StatisticsStatus::NonFiniteInput, "infinity rejected");
    check(summarizeStatistics(std::vector<double>{std::numeric_limits<double>::quiet_NaN()}).status ==
          StatisticsStatus::NonFiniteInput, "nan rejected");
    check(calculatePercentile(std::vector<double>{1,2}, -1).status ==
          StatisticsStatus::InvalidPercentile, "negative percentile rejected");
    check(calculatePercentile(std::vector<double>{1,2}, 101).status ==
          StatisticsStatus::InvalidPercentile, "percentile > 100 rejected");
    check(calculatePercentile(std::vector<double>{1,2}, std::numeric_limits<double>::quiet_NaN()).status ==
          StatisticsStatus::InvalidPercentile, "nan percentile rejected");
    check(calculatePercentile(std::vector<double>{1,std::numeric_limits<double>::infinity()}, 50).status ==
          StatisticsStatus::NonFiniteInput, "nonfinite percentile data rejected");
    check(std::isnan(summarizeStatistics(std::vector<double>{}).mean),
          "failed summary does not publish mean");
    check(std::isnan(calculatePercentile({},50).value),
          "failed percentile does not publish value");

    const auto huge = summarizeStatistics(std::vector<double>{1e308, 1e308});
    check(huge.status == StatisticsStatus::NonFiniteResult && std::isnan(huge.mean),
          "unrepresentable double sum rejected");
    const auto extreme = summarizeStatistics(std::vector<double>{-1e308,1e308});
    check(extreme.status == StatisticsStatus::NonFiniteResult,
          "unrepresentable double range or variance rejected");
    const std::vector<double> extremePercentileValues{-1e308, 1e308};
    const auto extremeP0 = calculatePercentile(extremePercentileValues, 0);
    const auto extremeP25 = calculatePercentile(extremePercentileValues, 25);
    const auto extremeP50 = calculatePercentile(extremePercentileValues, 50);
    const auto extremeP75 = calculatePercentile(extremePercentileValues, 75);
    const auto extremeP100 = calculatePercentile(extremePercentileValues, 100);
    check(extremeP0.successful() && extremeP0.value == -1e308,
          "extreme percentile lower endpoint");
    check(extremeP25.successful() && near(extremeP25.value, -5e307L),
          "extreme percentile lower quartile");
    check(extremeP50.successful() && near(extremeP50.value, 0),
          "percentile intermediate avoids double overflow");
    check(extremeP75.successful() && near(extremeP75.value, 5e307L),
          "extreme percentile upper quartile");
    check(extremeP100.successful() && extremeP100.value == 1e308,
          "extreme percentile upper endpoint");

    std::vector<double> over(kMaxStatisticsSamples + 1, 0.0);
    check(summarizeStatistics(over).status == StatisticsStatus::TooManySamples,
          "summary sample ceiling");
    check(calculatePercentile(over,50).status == StatisticsStatus::TooManySamples,
          "percentile sample ceiling");
    over.clear(); over.shrink_to_fit();

    const auto offset = summarizeStatistics(std::vector<double>{1e12+1,1e12+2,1e12+3,1e12+4});
    check(offset.successful() && near(offset.mean,1e12L+2.5L,1e-14L),
          "large-offset mean");
    check(offset.successful() && near(offset.populationVariance,1.25L,1e-11L),
          "large-offset stable population variance");
    check(offset.successful() && near(offset.sampleVariance,5.0L/3.0L,1e-11L),
          "large-offset stable sample variance");
    const auto cancellation = summarizeStatistics(std::vector<double>{1e16,1,-1e16,3});
    check(cancellation.successful() && cancellation.sum == 4,
          "compensated sum with cancellation");

    std::mt19937_64 generator(0x51A7157ULL);
    std::uniform_int_distribution<int> size(2, 40), value(-100,100);
    for (int trial=0; trial<160; ++trial) {
        std::vector<double> samples;
        const int n = size(generator);
        samples.reserve(static_cast<std::size_t>(n));
        for (int i=0;i<n;++i) samples.push_back(static_cast<double>(value(generator)));
        // Independent centered two-pass reference in long double.
        long double sum = 0;
        for (double v : samples) sum += static_cast<long double>(v);
        const long double mean = sum / n;
        long double sumsq = 0;
        for (double v : samples) {
            const long double delta = static_cast<long double>(v) - mean;
            sumsq += delta*delta;
        }
        auto actual = summarizeStatistics(samples);
        check(actual.successful(), "fixed-seed summary status");
        check(actual.successful() && near(actual.sum,sum), "fixed-seed sum oracle");
        check(actual.successful() && near(actual.mean,mean), "fixed-seed mean oracle");
        check(actual.successful() && near(actual.populationVariance,sumsq/n),
              "fixed-seed population variance oracle");
        check(actual.successful() && near(actual.sampleVariance,sumsq/(n-1)),
              "fixed-seed sample variance oracle");
        check(actual.successful() && near(actual.populationStdDev,std::sqrt(sumsq/n)),
              "fixed-seed population standard deviation oracle");
        std::sort(samples.begin(),samples.end());
        const long double expectedMedian =
            (static_cast<long double>(samples[static_cast<std::size_t>((n-1)/2)]) +
             samples[static_cast<std::size_t>(n/2)])/2.0L;
        check(actual.successful() && near(actual.median,expectedMedian),
              "fixed-seed median oracle");
        const auto q = calculatePercentile(samples,25);
        const long double rank=static_cast<long double>(n-1)*0.25L;
        const auto ix=static_cast<std::size_t>(rank);
        const long double fractional=rank-ix;
        const long double oracle=static_cast<long double>(samples[ix]) +
            fractional*(static_cast<long double>(samples[std::min(ix+1,samples.size()-1)])-samples[ix]);
        check(q.successful() && near(q.value,oracle),"fixed-seed percentile oracle");
    }
    check(std::string(statisticsStatusName(StatisticsStatus::NonFiniteResult))==
          "non-finite result", "status names");
    std::cout << (failures ? "FAIL: " : "PASS: ") << checks
              << " DescriptiveStatistics assertions\n";
    return failures ? 1 : 0;
}
