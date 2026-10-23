#include "BenchmarkMetrics.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace rationalstudio::benchmark;
namespace {
int checks = 0;
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
    ++checks;
}
}
int main() {
    try {
        const auto stats = summarize({1.0, 2.0, 3.0, 4.0, 5.0});
        check(stats.samples == 5, "sample count");
        check(stats.minMs == 1.0 && stats.maxMs == 5.0, "minimum/maximum");
        check(stats.medianMs == 3.0, "median");
        check(std::abs(stats.p95Ms - 4.8) < 1e-9, "linear p95 interpolation");
        check(stats.meanMs == 3.0, "mean");
        check(percentile({1.0}, 0.95) == 1.0, "singleton percentile");
        check(escapeCsv("plain") == "plain", "plain CSV");
        check(escapeCsv("a,b") == "\"a,b\"", "CSV comma");
        check(escapeCsv("a\"b") == "\"a\"\"b\"", "CSV quote");
        check(escapeJson("\"\\\n") == "\\\"\\\\\\n", "JSON escaping");
        bool rejected = false;
        try { (void)summarize({0, -1}); } catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "negative duration rejected");
        rejected = false;
        try { (void)summarize({}); } catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "empty sample rejected");
        const auto tmp = std::filesystem::temp_directory_path() /
            ("rationalstudio-issue25-metrics-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
        writeMeasurements(tmp, "smoke", {{"cold", "quoted,scenario", 4, 0, 2.5, 42, 4, 20, false},
                                           {"cold", "quoted,scenario", 4, 1, 3.5, 48, 4, 21, true}});
        std::ifstream csv(tmp / "smoke.csv"), json(tmp / "smoke.json");
        check(csv.good() && json.good(), "CSV/JSON emitted");
        std::string line, data;
        while (std::getline(json, line)) data += line;
        check(data.find("\"median_ms\": 3.000000") != std::string::npos,
              "JSON summary median");
        check(data.find("\"truncated_samples\": 1") != std::string::npos,
              "JSON truncation count");
        data.clear();
        while (std::getline(csv, line)) data += line;
        check(data.find("\"quoted,scenario\"") != std::string::npos, "quoted CSV output");
        std::filesystem::remove_all(tmp);
        std::cout << "PASS: " << checks << " GraphBenchmarkStatistics assertions\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return 1;
    }
}
