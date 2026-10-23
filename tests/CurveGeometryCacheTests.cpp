#include "CurveGeometryCache.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using rationalstudio::CurveGeometryCache;
using rationalstudio::Expression;

namespace {
int assertions = 0;
void require(bool ok, const char* description) {
    if (!ok) throw std::runtime_error(description);
    ++assertions;
}
std::shared_ptr<const Expression> expression(const std::string& text) {
    auto value = std::make_shared<Expression>(text);
    require(value->valid(), "test expression parses");
    return value;
}
} // namespace

int main() {
    try {
        using Cache = CurveGeometryCache<std::vector<int>>;
        Cache cache;
        int builds = 0;
        const auto a = expression("sin(x)");
        const auto b = expression("x^2");
        const auto c = expression("sin(x)"); // same source, different object

        cache.synchronize({a, b});
        require(cache.size() == 2, "two active entries");
        require(!cache.isReady(0) && !cache.isReady(1), "entries initially dirty");

        auto build = [&](int id) {
            return [&, id] {
                ++builds;
                return std::vector<int>{id, builds};
            };
        };
        require(cache.getOrBuild(0, build(10))[0] == 10, "first geometry built");
        require(cache.getOrBuild(0, build(99))[0] == 10, "repaint reuses first geometry");
        require(builds == 1, "repeat access never rebuilds");
        require(cache.getOrBuild(1, build(20))[0] == 20, "second geometry built");
        require(builds == 2, "both keys built once");

        // A pure metadata/visibility change preserves the two paths.
        cache.synchronize({a, b});
        require(cache.isReady(0) && cache.isReady(1), "same keys preserve entries");
        require(cache.getOrBuild(1, build(99))[0] == 20, "metadata update retains cache");
        require(builds == 2, "metadata never forces sampling");

        // Reordering should preserve identity, not index-based geometry.
        cache.synchronize({b, a});
        require(cache.getOrBuild(0, build(99))[0] == 20, "reorder keeps b");
        require(cache.getOrBuild(1, build(99))[0] == 10, "reorder keeps a");
        require(builds == 2, "reorder does not resample");

        // View, angle and ans changes all call invalidateAll from GraphCanvas.
        for (int i = 0; i < 4; ++i) {
            cache.invalidateAll();
            require(!cache.isReady(0) && !cache.isReady(1), "invalidation marks dirty");
            require(cache.getOrBuild(0, build(20))[0] == 20, "dirty first rebuilt");
            require(cache.getOrBuild(0, build(99))[0] == 20, "rebuilt first reused");
            require(cache.getOrBuild(1, build(10))[0] == 10, "dirty second rebuilt");
            require(builds == 4 + 2 * i, "exactly two rebuilds per invalidation");
        }

        // Text changes replace expression identity. Same display name is
        // insufficient to reuse another object's sampled geometry.
        cache.synchronize({b, c});
        require(cache.isReady(0) && !cache.isReady(1), "only changed expression invalidated");
        require(cache.getOrBuild(1, build(30))[0] == 30, "replacement built once");
        require(builds == 11, "new expression requires regeneration");

        // Hidden curves stay lazy and can keep a previously constructed path.
        cache.synchronize({b, c});
        require(cache.getOrBuild(1, build(99))[0] == 30, "unhidden curve reuses path");
        require(builds == 11, "hiding and showing did not resample");

        // Duplicate keys have one cache entry each; both get exactly one
        // old entry when the list is rebuilt and retain their own paths.
        cache.synchronize({b, b});
        require(cache.isReady(0) && !cache.isReady(1), "duplicate identity does not double-match");
        require(cache.getOrBuild(1, build(40))[0] == 40, "second duplicate built separately");
        require(builds == 12, "duplicate built once");
        cache.synchronize({b, b});
        require(cache.isReady(0) && cache.isReady(1), "duplicate entries stay cached");
        require(cache.getOrBuild(1, build(99))[0] == 40, "duplicate path retained");

        // Removing curves purges obsolete entries; empty list uses no memory.
        cache.synchronize({b});
        require(cache.size() == 1 && cache.isReady(0), "remove shrinks cache");
        cache.synchronize({});
        require(cache.size() == 0, "removing all curves clears cache");
        cache.synchronize({a});
        require(!cache.isReady(0), "removed expression readded must rebuild");
        require(cache.getOrBuild(0, build(50))[0] == 50, "readded expression re-evaluated");

        // Invalid functions and exceptions are cached as caller decisions.
        const auto bad = std::make_shared<Expression>("sin(");
        require(!bad->valid(), "invalid expression fixture");
        cache.synchronize({bad});
        int attempts = 0;
        try {
            (void)cache.getOrBuild(0, [&]() -> std::vector<int> {
                ++attempts;
                throw std::runtime_error("construction failure");
            });
            require(false, "exception must propagate");
        } catch (const std::runtime_error&) {
            require(!cache.isReady(0), "failed build remains dirty");
        }
        require(cache.getOrBuild(0, [&] { ++attempts; return std::vector<int>{}; }).empty(),
                "empty geometry can be cached");
        require(cache.isReady(0) && attempts == 2, "empty result not retried");
        (void)cache.getOrBuild(0, [&] { ++attempts; return std::vector<int>{5}; });
        require(attempts == 2, "empty result is a hit");

        std::cout << "PASS: " << assertions << " CurveGeometryCache assertions\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
