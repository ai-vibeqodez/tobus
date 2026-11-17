#include <catch2/catch_test_macros.hpp>

#include <aw/tobus/tobus.hpp>
#include <aw/tobus/transports/in_process_bus.hpp>

#include <aw/tpl/threading/tasks/async_sleep.hpp>

#include <atomic>
#include <chrono>
#include <exception>
#include <memory>
#include <semaphore>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
using namespace aw::tobus;

namespace {

struct Ping {
    int n = 0;
};

struct Pong {
    int n = 0;
};

// Counts copies: proves the in-process path hands out one shared object.
struct Tracked {
    static inline std::atomic<int> copies{0};
    int n = 0;
    explicit Tracked(int v)
        : n(v) {}
    Tracked(const Tracked& o)
        : n(o.n) {
        ++copies;
    }
    Tracked(Tracked&& o) noexcept
        : n(o.n) {}
};

} // namespace

TEST_CASE("publish delivers to a subscriber", "[in_process]") {
    InProcessBus bus;
    std::atomic<int> got{0};
    auto sub = bus.subscribe<Ping>([&](const Ping& p) -> Task<void> {
        got = p.n;
        co_return;
    });
    bus.publish(Ping{5});
    bus.drain().wait();
    REQUIRE(got == 5);
}

TEST_CASE("publish fans out to every subscriber of the type only", "[in_process]") {
    InProcessBus bus;
    std::atomic<int> pings{0};
    std::atomic<int> pongs{0};
    auto a = bus.subscribe<Ping>([&](const Ping&) -> Task<void> {
        ++pings;
        co_return;
    });
    auto b = bus.subscribe<Ping>([&](const Ping&) -> Task<void> {
        ++pings;
        co_return;
    });
    auto c = bus.subscribe<Pong>([&](const Pong&) -> Task<void> {
        ++pongs;
        co_return;
    });
    bus.publish(Ping{});
    bus.drain().wait();
    REQUIRE(pings == 2);
    REQUIRE(pongs == 0);
}

TEST_CASE("publishing without subscribers is a no-op", "[in_process]") {
    InProcessBus bus;
    bus.publish(Ping{});
    bus.drain().wait();
    SUCCEED();
}

TEST_CASE("drain on an idle bus completes", "[in_process]") {
    InProcessBus bus;
    bus.drain().wait();
    REQUIRE(bus.transport_name() == "in_process");
}

TEST_CASE("events are not copied per subscriber (no serialization, one shared object)", "[in_process]") {
    InProcessBus bus;
    std::vector<const Tracked*> seen(3, nullptr);
    std::atomic<int> idx{0};
    auto make = [&] {
        return bus.subscribe<Tracked>([&](const Tracked& t) -> Task<void> {
            seen[static_cast<std::size_t>(idx++)] = &t;
            co_return;
        });
    };
    auto s1 = make();
    auto s2 = make();
    auto s3 = make();
    Tracked::copies = 0;
    bus.publish(Tracked{1}); // moved into shared storage
    bus.drain().wait();
    REQUIRE(Tracked::copies == 0);
    REQUIRE(seen[0] == seen[1]);
    REQUIRE(seen[1] == seen[2]);
}

TEST_CASE("an lvalue event is copied exactly once, regardless of subscriber count", "[in_process]") {
    InProcessBus bus;
    auto s1 = bus.subscribe<Tracked>([](const Tracked&) -> Task<void> { co_return; });
    auto s2 = bus.subscribe<Tracked>([](const Tracked&) -> Task<void> { co_return; });
    Tracked t{1};
    Tracked::copies = 0;
    bus.publish(t);
    bus.drain().wait();
    REQUIRE(Tracked::copies == 1);
}

TEST_CASE("publish does not wait for handlers", "[in_process]") {
    InProcessBus bus;
    std::atomic<bool> release{false};
    std::atomic<bool> finished{false};
    auto sub = bus.subscribe<Ping>([&](const Ping&) -> Task<void> {
        const auto deadline = std::chrono::steady_clock::now() + 10s;
        while (!release && std::chrono::steady_clock::now() < deadline) {
            co_await aw::tpl::threading::tasks::async_sleep(1ms);
        }
        finished = true;
    });
    bus.publish(Ping{});
    REQUIRE_FALSE(finished); // publish returned while the handler is still parked
    release = true;
    bus.drain().wait();
    REQUIRE(finished);
}

TEST_CASE("handlers run concurrently", "[in_process]") {
    InProcessBus bus;
    std::atomic<int> started{0};
    std::atomic<bool> overlapped{false};
    auto handler = [&](const Ping&) -> Task<void> {
        ++started;
        const auto deadline = std::chrono::steady_clock::now() + 10s;
        while (started < 2 && std::chrono::steady_clock::now() < deadline) {
            co_await aw::tpl::threading::tasks::async_sleep(1ms);
        }
        if (started >= 2) {
            overlapped = true;
        }
    };
    auto a = bus.subscribe<Ping>(handler);
    auto b = bus.subscribe<Ping>(handler);
    bus.publish(Ping{});
    bus.drain().wait();
    REQUIRE(overlapped); // each handler saw the other one running at the same time
}

TEST_CASE("the event stays alive while a suspended handler runs", "[in_process]") {
    InProcessBus bus;
    std::atomic<int> seen{0};
    auto sub = bus.subscribe<Ping>([&](const Ping& p) -> Task<void> {
        co_await aw::tpl::threading::tasks::async_sleep(30ms);
        seen = p.n; // would read freed memory if the bus did not own the event
    });
    {
        Ping local{99};
        bus.publish(local);
        local.n = -1; // the caller's object is gone/changed by the time the handler resumes
    }
    bus.drain().wait();
    REQUIRE(seen == 99);
}

TEST_CASE("a failing handler is reported and does not affect others", "[in_process][errors]") {
    std::atomic<int> reported{0};
    std::string message;
    EventBusConfig cfg;
    cfg.on_error = [&](std::exception_ptr ep) {
        try {
            std::rethrow_exception(ep);
        } catch (const std::exception& e) {
            message = e.what();
        }
        ++reported;
    };
    InProcessBus bus(cfg);
    std::atomic<int> ok{0};
    auto bad = bus.subscribe<Ping>([](const Ping&) -> Task<void> {
        throw std::runtime_error("handler boom");
        co_return;
    });
    auto good = bus.subscribe<Ping>([&](const Ping&) -> Task<void> {
        ++ok;
        co_return;
    });
    bus.publish(Ping{});
    bus.drain().wait();
    REQUIRE(reported == 1);
    REQUIRE(message == "handler boom");
    REQUIRE(ok == 1);
}

TEST_CASE("a handler that throws before returning its task is reported", "[in_process][errors]") {
    std::atomic<int> reported{0};
    EventBusConfig cfg;
    cfg.on_error = [&](std::exception_ptr) { ++reported; };
    InProcessBus bus(cfg);
    auto sub = bus.subscribe<Ping>([](const Ping&) -> Task<void> { throw std::logic_error("sync"); });
    bus.publish(Ping{});
    bus.drain().wait();
    REQUIRE(reported == 1);
}

TEST_CASE("handler failures are ignored without an error handler, and a throwing error handler is contained",
          "[in_process][errors]") {
    {
        InProcessBus bus;
        auto sub = bus.subscribe<Ping>([](const Ping&) -> Task<void> {
            throw std::runtime_error("x");
            co_return;
        });
        bus.publish(Ping{});
        bus.drain().wait();
    }
    {
        EventBusConfig cfg;
        cfg.on_error = [](std::exception_ptr) { throw std::runtime_error("error handler failed"); };
        InProcessBus bus(cfg);
        auto sub = bus.subscribe<Ping>([](const Ping&) -> Task<void> {
            throw std::runtime_error("x");
            co_return;
        });
        bus.publish(Ping{});
        bus.drain().wait();
    }
    SUCCEED();
}

TEST_CASE("an empty handler is rejected", "[in_process][errors]") {
    InProcessBus bus;
    REQUIRE_THROWS_AS(bus.subscribe<Ping>(EventHandler<Ping>{}), std::invalid_argument);
}

TEST_CASE("handlers may publish: drain waits for the whole cascade", "[in_process]") {
    InProcessBus bus;
    std::atomic<int> pongs{0};
    auto relay = bus.subscribe<Ping>([&](const Ping& p) -> Task<void> {
        co_await aw::tpl::threading::tasks::async_sleep(10ms);
        bus.publish(Pong{p.n});
    });
    auto sink = bus.subscribe<Pong>([&](const Pong& p) -> Task<void> {
        pongs += p.n;
        co_return;
    });
    bus.publish(Ping{3});
    bus.drain().wait();
    REQUIRE(pongs == 3);
}

TEST_CASE("concurrent publishers deliver every event exactly once", "[in_process][threads]") {
    InProcessBus bus;
    std::atomic<int> a{0};
    std::atomic<int> b{0};
    auto s1 = bus.subscribe<Ping>([&](const Ping&) -> Task<void> {
        ++a;
        co_return;
    });
    auto s2 = bus.subscribe<Ping>([&](const Ping&) -> Task<void> {
        ++b;
        co_return;
    });
    constexpr int kThreads = 4;
    constexpr int kPerThread = 500;
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < kPerThread; ++i) {
                bus.publish(Ping{i});
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    bus.drain().wait();
    REQUIRE(a == kThreads * kPerThread);
    REQUIRE(b == kThreads * kPerThread);
}

TEST_CASE("subscribing and unsubscribing while publishing is safe", "[in_process][threads]") {
    InProcessBus bus;
    std::atomic<bool> stop{false};
    std::atomic<int> got{0};
    std::thread publisher([&] {
        while (!stop) {
            bus.publish(Ping{});
        }
    });
    for (int i = 0; i < 200; ++i) {
        auto sub = bus.subscribe<Ping>([&](const Ping&) -> Task<void> {
            ++got;
            co_return;
        });
    }
    stop = true;
    publisher.join();
    bus.drain().wait();
    SUCCEED();
}

namespace {
struct CollideA {};
struct CollideB {};
} // namespace

template <>
struct aw::tobus::EventTraits<CollideA> {
    static constexpr std::string_view name = "test.Collide";
};
template <>
struct aw::tobus::EventTraits<CollideB> {
    static constexpr std::string_view name = "test.Collide";
};

TEST_CASE("two different types with the same declared name are rejected", "[in_process][errors]") {
    InProcessBus bus;
    auto a = bus.subscribe<CollideA>([](const CollideA&) -> Task<void> { co_return; });
    REQUIRE_THROWS_AS(bus.subscribe<CollideB>([](const CollideB&) -> Task<void> { co_return; }), std::logic_error);
}
