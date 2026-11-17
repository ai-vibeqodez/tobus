// Every stub header must compile on its own and be well-formed, independent of the TOBUS_ENABLE_* options.
#include <catch2/catch_test_macros.hpp>

#include <aw/tobus/transports/framed_bus.hpp>
#include <aw/tobus/transports/http_bus.hpp>
#include <aw/tobus/transports/kafka_bus.hpp>
#include <aw/tobus/transports/rabbitmq_bus.hpp>
#include <aw/tobus/transports/redis_bus.hpp>
#include <aw/tobus/transports/tcp_server_bus.hpp>
#include <aw/tobus/transports/unix_socket_bus.hpp>
#include <aw/tobus/transports/websocket_bus.hpp>
#include <aw/tobus/transports/zeromq_bus.hpp>

#include <aw/tobus/tobus.hpp>

#include <type_traits>

using namespace aw::tobus;

namespace {

struct Plain {};
struct Wire {
    int v = 0;
};

} // namespace

template <>
struct aw::tobus::EventTraits<Wire> {
    static constexpr std::string_view name = "test.StubWire";
    static std::vector<std::byte> serialize(const Wire&) { return {}; }
    static Wire deserialize(std::span<const std::byte>) { return {}; }
};

namespace {

// Test double: exposes the protected FramedBus surface.
class Probe final : public FramedBus {
public:
    Probe()
        : FramedBus(EventBusConfig{}) {}
    [[nodiscard]] std::string_view transport_name() const noexcept override { return "probe"; }
    using FramedBus::receive_frame;
    using FramedBus::require_serializable;
    void send_frame(const Frame&) override {}
    using FramedBus::publish_raw;
    using FramedBus::subscribe_raw;
};

} // namespace

TEST_CASE("every transport stub is an IEventBus", "[stubs]") {
    STATIC_REQUIRE(std::is_base_of_v<IEventBus, UnixSocketBus>);
    STATIC_REQUIRE(std::is_base_of_v<IEventBus, TCPServerBus>);
    STATIC_REQUIRE(std::is_base_of_v<IEventBus, HTTPBus>);
    STATIC_REQUIRE(std::is_base_of_v<IEventBus, WebSocketBus>);
    STATIC_REQUIRE(std::is_base_of_v<IEventBus, RedisBus>);
    STATIC_REQUIRE(std::is_base_of_v<IEventBus, ZeroMQBus>);
    STATIC_REQUIRE(std::is_base_of_v<IEventBus, RabbitMQBus>);
    STATIC_REQUIRE(std::is_base_of_v<IEventBus, KafkaBus>);
}

TEST_CASE("the header-only broker stubs throw TransportNotImplemented", "[stubs]") {
    REQUIRE_THROWS_AS(RedisBus(EventBusConfig{}), TransportNotImplemented);
    REQUIRE_THROWS_AS(ZeroMQBus(EventBusConfig{}), TransportNotImplemented);
    REQUIRE_THROWS_AS(RabbitMQBus(EventBusConfig{}), TransportNotImplemented);
    REQUIRE_THROWS_AS(KafkaBus(EventBusConfig{}), TransportNotImplemented);
}

#if TOBUS_HAS_UNIX_SOCKET && TOBUS_HAS_TCP_SERVER && TOBUS_HAS_HTTP && TOBUS_HAS_WEBSOCKET
TEST_CASE("the socket stubs throw TransportNotImplemented", "[stubs]") {
    REQUIRE_THROWS_AS(UnixSocketBus(EventBusConfig{}), TransportNotImplemented);
    REQUIRE_THROWS_AS(TCPServerBus(EventBusConfig{}), TransportNotImplemented);
    REQUIRE_THROWS_AS(HTTPBus(EventBusConfig{}), TransportNotImplemented);
    REQUIRE_THROWS_AS(WebSocketBus(EventBusConfig{}), TransportNotImplemented);
}
#endif

TEST_CASE("FramedBus foundation: origin ids, serializability check, TODO bodies", "[stubs]") {
    Probe a;
    Probe b;
    REQUIRE(a.origin_id() != b.origin_id());

    // Non-serializable types are rejected (this is what subscribe() will use on a network transport).
    REQUIRE_THROWS_AS(Probe::require_serializable(event_type_info<Plain>()), NotSerializable);
    REQUIRE_NOTHROW(Probe::require_serializable(event_type_info<Wire>()));

    // The transport-agnostic parts are still TODO and say so.
    REQUIRE_THROWS_AS(a.drain(), TransportNotImplemented);
    REQUIRE_THROWS_AS(a.publish_raw(event_type_info<Wire>(), std::make_shared<const Wire>()), TransportNotImplemented);
    REQUIRE_THROWS_AS(a.subscribe_raw(event_type_info<Wire>(), [](const void*) -> Task<void> { co_return; }),
                      TransportNotImplemented);
    REQUIRE_THROWS_AS(a.receive_frame(FramedBus::Frame{}), TransportNotImplemented);
}

TEST_CASE("error messages are actionable", "[stubs]") {
    REQUIRE(std::string(TransportNotAvailable(TransportKind::redis).what()).find("TOBUS_ENABLE_REDIS") !=
            std::string::npos);
    REQUIRE(std::string(NotSerializable("x.Y").what()).find("x.Y") != std::string::npos);
    REQUIRE(std::string(TransportNotImplemented("Foo").what()).find("Foo") != std::string::npos);
}
