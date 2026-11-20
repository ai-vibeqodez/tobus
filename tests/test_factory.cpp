#include <catch2/catch_test_macros.hpp>

#include <aw/tobus/tobus.hpp>

#include <string>

using namespace aw::tobus;

TEST_CASE("TransportKind names round-trip", "[factory]") {
    for (const TransportKind kind : all_transport_kinds) {
        const auto parsed = transport_from_string(to_string(kind));
        REQUIRE(parsed.has_value());
        REQUIRE(*parsed == kind);
    }
    REQUIRE_FALSE(transport_from_string("carrier_pigeon").has_value());
    REQUIRE(to_string(static_cast<TransportKind>(200)) == "unknown");
}

TEST_CASE("the factory creates an in-process bus by default", "[factory]") {
    auto bus = EventBusFactory::create();
    REQUIRE(bus != nullptr);
    REQUIRE(bus->transport_name() == "in_process");
    REQUIRE(EventBusFactory::is_available(TransportKind::in_process));
}

TEST_CASE("the factory forwards on_error to the bus", "[factory]") {
    struct Boom {};
    std::atomic<int> reported{0};
    EventBusConfig cfg;
    cfg.on_error = [&](const std::exception_ptr&) { ++reported; };
    auto bus = EventBusFactory::create(cfg);
    auto sub = bus->subscribe<Boom>([](const Boom&) -> Task<void> {
        throw std::runtime_error("x");
        co_return;
    });
    bus->publish(Boom{});
    bus->drain().wait();
    REQUIRE(reported == 1);
}

TEST_CASE("a transport that is not compiled in reports the CMake option", "[factory]") {
    for (const TransportKind kind : all_transport_kinds) {
        if (EventBusFactory::is_available(kind)) {
            continue;
        }
        EventBusConfig cfg;
        cfg.transport = kind;
        try {
            (void)EventBusFactory::create(cfg);
            FAIL("expected TransportNotAvailable for " << to_string(kind));
        } catch (const TransportNotAvailable& ex) {
            REQUIRE(std::string(ex.what()).find("TOBUS_ENABLE_") != std::string::npos);
        }
    }
}

TEST_CASE("an out-of-range transport kind is unavailable", "[factory]") {
    EventBusConfig cfg;
    cfg.transport = static_cast<TransportKind>(200);
    REQUIRE_FALSE(EventBusFactory::is_available(cfg.transport));
    REQUIRE_THROWS_AS(EventBusFactory::create(cfg), TransportNotAvailable);
}

TEST_CASE("a compiled-in stub transport reports that it is not implemented", "[factory]") {
    for (const TransportKind kind : all_transport_kinds) {
        if (kind == TransportKind::in_process || kind == TransportKind::http || !EventBusFactory::is_available(kind)) {
            continue;
        }
        EventBusConfig cfg;
        cfg.transport = kind;
        REQUIRE_THROWS_AS(EventBusFactory::create(cfg), TransportNotImplemented);
    }
}
