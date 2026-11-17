#include <catch2/catch_test_macros.hpp>

#include <aw/tobus/tobus.hpp>
#include <aw/tobus/transports/in_process_bus.hpp>

#include <atomic>
#include <memory>
#include <utility>

using namespace aw::tobus;

namespace {
struct Ping {};
} // namespace

TEST_CASE("destroying a Subscription unsubscribes", "[subscription]") {
    InProcessBus bus;
    std::atomic<int> got{0};
    {
        auto sub = bus.subscribe<Ping>([&](const Ping&) -> Task<void> {
            ++got;
            co_return;
        });
        bus.publish(Ping{});
        bus.drain().wait();
        REQUIRE(got == 1);
    }
    bus.publish(Ping{});
    bus.drain().wait();
    REQUIRE(got == 1);
}

TEST_CASE("cancel() unsubscribes and is idempotent", "[subscription]") {
    InProcessBus bus;
    std::atomic<int> got{0};
    auto sub = bus.subscribe<Ping>([&](const Ping&) -> Task<void> {
        ++got;
        co_return;
    });
    REQUIRE(sub.active());
    sub.cancel();
    sub.cancel();
    REQUIRE_FALSE(sub.active());
    bus.publish(Ping{});
    bus.drain().wait();
    REQUIRE(got == 0);
}

TEST_CASE("cancelling one subscription leaves the others", "[subscription]") {
    InProcessBus bus;
    std::atomic<int> a{0};
    std::atomic<int> b{0};
    auto sa = bus.subscribe<Ping>([&](const Ping&) -> Task<void> {
        ++a;
        co_return;
    });
    auto sb = bus.subscribe<Ping>([&](const Ping&) -> Task<void> {
        ++b;
        co_return;
    });
    sa.cancel();
    bus.publish(Ping{});
    bus.drain().wait();
    REQUIRE(a == 0);
    REQUIRE(b == 1);
}

TEST_CASE("release() keeps the handler registered", "[subscription]") {
    InProcessBus bus;
    std::atomic<int> got{0};
    {
        auto sub = bus.subscribe<Ping>([&](const Ping&) -> Task<void> {
            ++got;
            co_return;
        });
        sub.release();
        REQUIRE_FALSE(sub.active());
    }
    bus.publish(Ping{});
    bus.drain().wait();
    REQUIRE(got == 1);
}

TEST_CASE("Subscription is move-only and moving transfers ownership", "[subscription]") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<Subscription>);
    InProcessBus bus;
    std::atomic<int> got{0};
    auto sub = bus.subscribe<Ping>([&](const Ping&) -> Task<void> {
        ++got;
        co_return;
    });
    Subscription moved(std::move(sub));
    REQUIRE_FALSE(sub.active()); // NOLINT(bugprone-use-after-move): testing the moved-from state
    REQUIRE(moved.active());

    // Move-assigning over an active subscription cancels the old one.
    std::atomic<int> other{0};
    Subscription second = bus.subscribe<Ping>([&](const Ping&) -> Task<void> {
        ++other;
        co_return;
    });
    second = std::move(moved);
    bus.publish(Ping{});
    bus.drain().wait();
    REQUIRE(got == 1);
    REQUIRE(other == 0);

    Subscription& alias = second;
    second = std::move(alias); // self-move keeps it active
    REQUIRE(second.active());
}

TEST_CASE("a default Subscription is inactive and harmless", "[subscription]") {
    Subscription s;
    REQUIRE_FALSE(s.active());
    s.cancel();
}

TEST_CASE("a Subscription may outlive its bus", "[subscription]") {
    Subscription survivor;
    {
        auto bus = std::make_unique<InProcessBus>();
        survivor = bus->subscribe<Ping>([](const Ping&) -> Task<void> { co_return; });
        REQUIRE(survivor.active());
    }
    survivor.cancel(); // no crash: the dispatcher is gone
    SUCCEED();
}

TEST_CASE("a bus may be destroyed while deliveries are in flight", "[subscription]") {
    std::atomic<int> got{0};
    {
        InProcessBus bus;
        auto sub = bus.subscribe<Ping>([&](const Ping&) -> Task<void> {
            ++got;
            co_return;
        });
        bus.publish(Ping{});
        // no drain: in-flight tasks keep the dispatcher alive on their own
        bus.drain().wait();
    }
    REQUIRE(got == 1);
}
