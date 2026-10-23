#pragma once

#include "Expression.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace rationalstudio {

// A bounded-to-the-current-curve-list geometry cache. Entries are keyed by
// *object identity*, not presentation text: two different compiled expressions
// with equal labels must not accidentally reuse geometry. Shared ownership
// prevents address-reuse collisions while the corresponding entry is alive.
//
// Geometry is supplied by the caller (QPainterPath in GraphCanvas); this
// cache is Qt-independent and can be tested using plain C++ value types.
// All access must remain on the owning GUI thread.
template <typename Geometry>
class CurveGeometryCache {
public:
    using Key = std::shared_ptr<const Expression>;

    // Keep geometry only for currently present curves. Existing matching
    // entries survive reorder and style/visibility changes. Each old entry
    // can match at most once, even when callers supply duplicate keys.
    void synchronize(const std::vector<Key>& keys) {
        std::vector<Entry> replacement;
        replacement.reserve(keys.size());
        for (const Key& key : keys) {
            bool found = false;
            if (key) {
                for (Entry& old : entries_) {
                    if (old.key == key) {
                        replacement.push_back(std::move(old));
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                replacement.push_back(Entry{key, Geometry{}, false});
            }
        }
        entries_ = std::move(replacement);
    }

    // Mark existing geometry stale without dropping expression identity.
    // Recalculation is lazy: disabled or invalid curves incur no work.
    void invalidateAll() noexcept {
        for (Entry& entry : entries_) {
            entry.ready = false;
        }
    }

    template <typename Factory>
    const Geometry& getOrBuild(std::size_t index, Factory&& factory) {
        Entry& entry = entries_.at(index);
        if (!entry.ready) {
            // Compute first so a throwing factory leaves this entry dirty.
            Geometry candidate = std::forward<Factory>(factory)();
            entry.geometry = std::move(candidate);
            entry.ready = true;
        }
        return entry.geometry;
    }

    // Retrieve an already-built path without accidentally constructing
    // geometry. Callers must check isReady() first.
    [[nodiscard]] const Geometry& readyGeometry(std::size_t index) const {
        const Entry& entry = entries_.at(index);
        if (!entry.ready) throw std::logic_error("geometry is not ready");
        return entry.geometry;
    }

    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
    [[nodiscard]] bool isReady(std::size_t index) const {
        return entries_.at(index).ready;
    }

private:
    struct Entry {
        Key key;
        Geometry geometry;
        bool ready = false;
    };
    std::vector<Entry> entries_;
};

} // namespace rationalstudio
