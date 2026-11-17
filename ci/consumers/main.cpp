// Shared by the three consumer projects: proves that a downstream project can include tobus, link it (together with
// its aw::tpl dependency) and run a publish/subscribe round trip.
#include <aw/tobus/tobus.hpp>
#include <aw/tobus/transports/in_process_bus.hpp>

#include <atomic>
#include <iostream>

namespace {
struct Answer {
    int value = 0;
};
} // namespace

int main() {
    aw::tobus::InProcessBus bus;
    std::atomic<int> got{0};
    auto sub = bus.subscribe<Answer>([&](const Answer& a) -> aw::tobus::Task<void> {
        got = a.value;
        co_return;
    });
    bus.publish(Answer{42});
    bus.drain().wait();
    std::cout << "got " << got.load() << '\n';
    return got.load() == 42 ? 0 : 1;
}
