#pragma once

#include "GraphViewport.hpp"

#include <cmath>
#include <cstdint>
#include <optional>

namespace rationalstudio {

// Temporary display transform from an already-sampled (source) viewport
// to the latest (target) viewport. This does not change cached geometry.
// It is intentionally separate from resampling; zoomed-out previews can
// expose areas that were outside the original sampled range until refresh.
struct GraphScreenPreview {
    double ratio = 1.0;
    double tx = 0.0;
    double ty = 0.0;
};

[[nodiscard]] inline std::optional<GraphScreenPreview> graphScreenPreview(
    const GraphViewport& source, const GraphViewport& target) noexcept {
    if (!source.isValid() || !target.isValid()) return std::nullopt;
    const double ratio = target.pixelsPerUnit() / source.pixelsPerUnit();
    const double tx = std::fma(-ratio, source.width() * 0.5,
                      target.width() * 0.5 +
                      (source.centerX() - target.centerX()) * target.pixelsPerUnit());
    const double ty = std::fma(-ratio, source.height() * 0.5,
                      target.height() * 0.5 +
                      (target.centerY() - source.centerY()) * target.pixelsPerUnit());
    if (!std::isfinite(ratio) || !std::isfinite(tx) || !std::isfinite(ty))
        return std::nullopt;
    return GraphScreenPreview{ratio, tx, ty};
}

// Small, Qt-independent state machine for coalescing interactive viewport
// refreshes and invalidating obsolete deferred-paint requests. All methods
// are called from the owning GUI thread. No background work is implied.
class GraphRepaintScheduler {
public:
    static constexpr int kNavigationIntervalMs = 16;
    static constexpr int kDeferredIntervalMs = 16;

    // Each pointer/scroll change requires new viewport geometry, but only
    // the first event in the interval needs to arm the navigation timer.
    [[nodiscard]] bool navigationChanged() noexcept {
        ++generation_;
        const bool armTimer = !navigationPending_;
        navigationPending_ = true;
        deferredPending_ = false;
        ++navigationEvents_;
        if (!armTimer) ++coalescedEvents_;
        return armTimer;
    }

    [[nodiscard]] bool navigationPending() const noexcept {
        return navigationPending_;
    }

    // Called by the navigation timer. Regenerate the latest viewport once,
    // rather than scheduling work for each obsolete intermediate viewport.
    [[nodiscard]] bool takeNavigationRefresh() noexcept {
        if (!navigationPending_) return false;
        navigationPending_ = false;
        ++navigationRefreshes_;
        return true;
    }

    // An explicit viewport/configuration change supersedes pending queued
    // navigation/deferred refreshes and is handled by the caller immediately.
    void invalidate() noexcept {
        ++generation_;
        navigationPending_ = false;
        deferredPending_ = false;
    }

    [[nodiscard]] bool scheduleDeferred() noexcept {
        if (navigationPending_ || deferredPending_) return false;
        deferredPending_ = true;
        deferredGeneration_ = generation_;
        return true;
    }

    [[nodiscard]] bool takeDeferredRefresh() noexcept {
        if (!deferredPending_) return false;
        const bool current = !navigationPending_ &&
                             deferredGeneration_ == generation_;
        deferredPending_ = false;
        if (current) ++deferredRefreshes_;
        return current;
    }

    void cancelDeferred() noexcept { deferredPending_ = false; }

    [[nodiscard]] std::uint64_t generation() const noexcept {
        return generation_;
    }
    [[nodiscard]] std::uint64_t navigationEvents() const noexcept {
        return navigationEvents_;
    }
    [[nodiscard]] std::uint64_t coalescedEvents() const noexcept {
        return coalescedEvents_;
    }
    [[nodiscard]] std::uint64_t navigationRefreshes() const noexcept {
        return navigationRefreshes_;
    }
    [[nodiscard]] std::uint64_t deferredRefreshes() const noexcept {
        return deferredRefreshes_;
    }

private:
    std::uint64_t generation_ = 0;
    std::uint64_t deferredGeneration_ = 0;
    std::uint64_t navigationEvents_ = 0;
    std::uint64_t coalescedEvents_ = 0;
    std::uint64_t navigationRefreshes_ = 0;
    std::uint64_t deferredRefreshes_ = 0;
    bool navigationPending_ = false;
    bool deferredPending_ = false;
};

} // namespace rationalstudio
