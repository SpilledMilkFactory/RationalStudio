#pragma once

// Issue #59: restore the benchmark-statistics support header required by the
// existing RationalGraphBenchmarkStatisticsTests in clean, isolated builds.
// Benchmark timings are diagnostics, NOT performance acceptance thresholds.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace rationalstudio::benchmark {

struct BenchmarkSummary {
    std::size_t samples = 0;
    double minMs = 0.0;
    double maxMs = 0.0;
    double medianMs = 0.0;
    double p95Ms = 0.0;
    double meanMs = 0.0;
};

inline double percentile(std::vector<double> samples, double fraction) {
    if (samples.empty() || !std::isfinite(fraction) || fraction < 0.0 ||
        fraction > 1.0) {
        throw std::invalid_argument("invalid benchmark percentile input");
    }
    for (double elapsed : samples) {
        if (!std::isfinite(elapsed) || elapsed < 0.0) {
            throw std::invalid_argument("benchmark duration must be finite and nonnegative");
        }
    }
    std::sort(samples.begin(), samples.end());
    if (samples.size() == 1) return samples.front();
    const long double position = static_cast<long double>(fraction) *
        static_cast<long double>(samples.size() - 1);
    const auto lower = static_cast<std::size_t>(position);
    const auto upper = std::min(lower + 1, samples.size() - 1);
    const long double weight = position - static_cast<long double>(lower);
    return static_cast<double>((1 - weight) * samples[lower] + weight * samples[upper]);
}

inline BenchmarkSummary summarize(const std::vector<double>& samples) {
    if (samples.empty()) throw std::invalid_argument("no benchmark samples");
    for (double elapsed : samples) {
        if (!std::isfinite(elapsed) || elapsed < 0.0) {
            throw std::invalid_argument("benchmark duration must be finite and nonnegative");
        }
    }
    const auto [minimum, maximum] = std::minmax_element(samples.begin(), samples.end());
    long double total = 0.0L;
    for (double elapsed : samples) total += static_cast<long double>(elapsed);
    return {samples.size(), *minimum, *maximum, percentile(samples, 0.5),
            percentile(samples, 0.95),
            static_cast<double>(total / static_cast<long double>(samples.size()))};
}

inline std::string escapeCsv(const std::string& value) {
    if (value.find_first_of(",\"\n\r") == std::string::npos) return value;
    std::string escaped = "\"";
    for (char character : value) {
        if (character == '"') escaped += "\"\"";
        else escaped += character;
    }
    return escaped + '"';
}

inline std::string escapeJson(const std::string& value) {
    std::string escaped;
    for (unsigned char character : value) {
        switch (character) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (character < 0x20) {
                    constexpr char hex[] = "0123456789abcdef";
                    escaped += "\\u00";
                    escaped += hex[character >> 4];
                    escaped += hex[character & 15];
                } else escaped += static_cast<char>(character);
        }
    }
    return escaped;
}

struct BenchmarkMeasurement {
    std::string phase;
    std::string scenario;
    std::size_t curves = 0;
    std::size_t repetition = 0;
    double elapsedMs = 0.0;
    std::size_t evaluations = 0;
    std::size_t segments = 0;
    std::size_t vertices = 0;
    bool truncated = false;
};

inline void writeMeasurements(const std::filesystem::path& folder,
                              const std::string& name,
                              const std::vector<BenchmarkMeasurement>& measurements) {
    if (name.empty() || name == "." || name == ".." ||
        name.find_first_of("/\\") != std::string::npos) {
        throw std::invalid_argument("invalid benchmark output name");
    }
    if (measurements.empty()) throw std::invalid_argument("no benchmark measurements");
    std::vector<double> durations;
    std::size_t truncated = 0;
    for (const auto& measurement : measurements) {
        if (!std::isfinite(measurement.elapsedMs) || measurement.elapsedMs < 0.0) {
            throw std::invalid_argument("invalid elapsed duration");
        }
        durations.push_back(measurement.elapsedMs);
        truncated += measurement.truncated ? 1u : 0u;
    }
    const BenchmarkSummary summary = summarize(durations);
    std::filesystem::create_directories(folder);
    std::ofstream csv(folder / (name + ".csv"));
    std::ofstream json(folder / (name + ".json"));
    if (!csv || !json) throw std::runtime_error("unable to create benchmark outputs");
    csv << "phase,scenario,curves,repetition,elapsed_ms,evaluations,segments,vertices,truncated\n";
    csv << std::fixed << std::setprecision(6);
    for (const auto& measurement : measurements) {
        csv << escapeCsv(measurement.phase) << ',' << escapeCsv(measurement.scenario)
            << ',' << measurement.curves << ',' << measurement.repetition << ','
            << measurement.elapsedMs << ',' << measurement.evaluations << ','
            << measurement.segments << ',' << measurement.vertices << ','
            << (measurement.truncated ? "true" : "false") << '\n';
    }
    json << std::fixed << std::setprecision(6);
    json << "{\n  \"samples\": " << summary.samples
         << ",\n  \"min_ms\": " << summary.minMs
         << ",\n  \"max_ms\": " << summary.maxMs
         << ",\n  \"median_ms\": " << summary.medianMs
         << ",\n  \"p95_ms\": " << summary.p95Ms
         << ",\n  \"mean_ms\": " << summary.meanMs
         << ",\n  \"truncated_samples\": " << truncated
         << "\n}\n";
    if (!csv || !json) throw std::runtime_error("failed to write benchmark outputs");
}

}  // namespace rationalstudio::benchmark
