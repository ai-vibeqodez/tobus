// Stats module — pure observer: subscribes to everything, publishes nothing.
// Adding it required no change in any other module.

#pragma once

#include <atomic>
#include <aw/tobus/tobus.hpp>
#include "events.hpp"

using namespace aw::tobus;

namespace aw::tobus::examples::chat {

class Stats {
public:
    explicit Stats(IEventBus& bus);

    // Handlers run concurrently on different threads, hence atomics.
    std::atomic<int> joined{0};
    std::atomic<int> sent{0};
    std::atomic<int> approved{0};
    std::atomic<int> blocked{0};

private:
    Subscription joined_;
    Subscription sent_;
    Subscription approved_;
    Subscription blocked_;
};

} // namespace aw::tobus::examples::chat
