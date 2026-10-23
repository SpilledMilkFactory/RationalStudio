#include "GraphRepaintScheduler.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

using rationalstudio::GraphRepaintScheduler;

namespace {
int assertions = 0;
void require(bool condition, const char* what) {
    if (!condition) throw std::runtime_error(what);
    ++assertions;
}
}

int main() {
    try {
        // Preview mapping must agree with true world/screen transforms:
        // sample screen point -> source world -> target screen.
        for (int k = 0; k < 100; ++k) {
            rationalstudio::GraphViewport from, to;
            require(from.setDimensions(800, 600), "set source dimensions");
            require(to.setDimensions(800, 600), "set destination dimensions");
            const double shift = (k - 50) * 0.125;
            require(from.setView(shift, -shift * 0.3, 58.0),
                    "set source viewport");
            require(to.setView(shift + 0.1, -shift * 0.3 + 0.2,
                               58.0 * (1.0 + 0.01 * k)),
                    "set target viewport");
            const auto preview = rationalstudio::graphScreenPreview(from, to);
            require(preview.has_value(), "finite affine preview exists");
            for (int j = 0; j < 7; ++j) {
                const double px = 25 + j * 113.0;
                const double py = 30 + j * 83.0;
                const double x = from.screenToWorldX(px);
                const double y = from.screenToWorldY(py);
                require(std::abs(std::fma(px, preview->ratio, preview->tx) -
                                 to.worldToScreenX(x)) < 1.0e-8,
                        "preview x matches world-space projection");
                require(std::abs(std::fma(py, preview->ratio, preview->ty) -
                                 to.worldToScreenY(y)) < 1.0e-8,
                        "preview y matches world-space projection");
            }
        }
        GraphRepaintScheduler scheduler;
        require(!scheduler.navigationPending(), "initially no navigation work");
        require(!scheduler.takeNavigationRefresh(), "no phantom navigation refresh");
        require(!scheduler.takeDeferredRefresh(), "no phantom deferred work");
        require(GraphRepaintScheduler::kNavigationIntervalMs > 0,
                "navigation interval is finite and positive");
        require(GraphRepaintScheduler::kDeferredIntervalMs > 0,
                "deferred interval is finite and positive");

        const auto initial = scheduler.generation();
        require(scheduler.navigationChanged(), "first input arms a single refresh");
        require(scheduler.navigationPending(), "interactive refresh pending");
        require(scheduler.generation() == initial + 1, "first revision increments");
        for (int i = 0; i < 1000; ++i) {
            require(!scheduler.navigationChanged(), "rapid input coalesces");
            require(scheduler.navigationPending(), "pending throughout burst");
        }
        require(scheduler.navigationEvents() == 1001,
                "all navigation inputs accounted for");
        require(scheduler.coalescedEvents() == 1000,
                "all subsequent events were coalesced");
        require(scheduler.navigationRefreshes() == 0,
                "no refresh before timer callback");
        require(scheduler.takeNavigationRefresh(), "consume latest navigation state");
        require(!scheduler.navigationPending(), "no pending navigation after refresh");
        require(scheduler.navigationRefreshes() == 1,
                "one refresh for a thousand intermediate states");
        require(!scheduler.takeNavigationRefresh(), "no duplicate navigation refresh");

        require(scheduler.scheduleDeferred(), "queue deferred rebuild once");
        require(!scheduler.scheduleDeferred(), "repeat deferred request coalesces");
        require(scheduler.takeDeferredRefresh(), "deferred rebuild is current");
        require(scheduler.deferredRefreshes() == 1, "one deferred refresh");
        require(!scheduler.takeDeferredRefresh(), "consumed deferred request is gone");

        require(scheduler.scheduleDeferred(), "new deferred request queued");
        const auto beforeChange = scheduler.generation();
        require(scheduler.navigationChanged(),
                "navigation supersedes old pending rebuild");
        require(scheduler.generation() > beforeChange, "revision increments");
        require(!scheduler.takeDeferredRefresh(),
                "discard obsolete deferred repaint");
        require(!scheduler.scheduleDeferred(),
                "no deferred work while previewing interaction");
        require(scheduler.takeNavigationRefresh(),
                "refresh current viewport after superseding job");

        require(scheduler.scheduleDeferred(), "queue work before invalidation");
        const auto epoch = scheduler.generation();
        scheduler.invalidate();
        require(scheduler.generation() == epoch + 1,
                "explicit invalidation creates new generation");
        require(!scheduler.takeDeferredRefresh(),
                "explicit invalidation cancels pending deferred work");
        require(!scheduler.navigationPending(),
                "explicit invalidation clears navigation");

        require(scheduler.navigationChanged(), "start navigation after invalidation");
        scheduler.invalidate();
        require(!scheduler.takeNavigationRefresh(),
                "explicit update supersedes queued navigation refresh");
        require(scheduler.scheduleDeferred(), "queue final deferred request");
        scheduler.cancelDeferred();
        require(!scheduler.takeDeferredRefresh(), "deferred cancellation is final");

        // Deterministic event-sequence stress: repeated navigation bursts
        // must need only one committed refresh per burst, independent of
        // how many intermediate viewport states become obsolete.
        constexpr int kBursts = 128;
        for (int b = 0; b < kBursts; ++b) {
            const auto previous = scheduler.navigationRefreshes();
            require(scheduler.navigationChanged(), "fresh burst arms timer");
            for (int j = 0; j < (b % 19) + 1; ++j) {
                require(!scheduler.navigationChanged(),
                        "further inputs don't arm more timers");
            }
            require(scheduler.takeNavigationRefresh(), "latest burst processed");
            require(scheduler.navigationRefreshes() == previous + 1,
                    "one refresh per navigation burst");
            require(!scheduler.navigationPending(), "burst drained");
            require(scheduler.scheduleDeferred(), "deferred work after burst");
            require(scheduler.takeDeferredRefresh(), "deferred work eventually serviced");
        }
        std::cout << "PASS: " << assertions << " GraphRepaintScheduler assertions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
