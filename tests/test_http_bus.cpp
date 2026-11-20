#include <catch2/catch_test_macros.hpp>

#include <aw/tobus/transports/http_bus.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using namespace aw::tobus;

namespace {

struct HttpEvent {
    std::uint32_t value = 0;
};

} // namespace

template <>
struct aw::tobus::EventTraits<HttpEvent> {
    static constexpr std::string_view name = "test.HTTPEvent";
    static std::vector<std::byte> serialize(const HttpEvent& event) {
        return {static_cast<std::byte>((event.value >> 24) & 0xffU),
                static_cast<std::byte>((event.value >> 16) & 0xffU), static_cast<std::byte>((event.value >> 8) & 0xffU),
                static_cast<std::byte>(event.value & 0xffU)};
    }
    static HttpEvent deserialize(std::span<const std::byte> data) {
        if (data.size() != 4) throw std::runtime_error("invalid test event");
        return {static_cast<std::uint32_t>(std::to_integer<unsigned char>(data[0])) << 24 |
                static_cast<std::uint32_t>(std::to_integer<unsigned char>(data[1])) << 16 |
                static_cast<std::uint32_t>(std::to_integer<unsigned char>(data[2])) << 8 |
                static_cast<std::uint32_t>(std::to_integer<unsigned char>(data[3]))};
    }
};

#if TOBUS_HAS_HTTP
TEST_CASE("HTTPBus relays events between client and server", "[transport][http]") {
    EventBusConfig server_config;
    server_config.transport = TransportKind::http;
    server_config.endpoint = "127.0.0.1:39173";
    server_config.options["role"] = "server";
    HTTPBus server(server_config);

    EventBusConfig client_config;
    client_config.transport = TransportKind::http;
    client_config.endpoint = "http://127.0.0.1:39173";
    client_config.options["role"] = "client";
    HTTPBus client(client_config);

    std::atomic<std::uint32_t> received{0};
    std::atomic<std::uint32_t> received_by_server{0};
    auto server_subscription = server.subscribe<HttpEvent>([&](const HttpEvent& event) -> Task<void> {
        received_by_server.store(event.value, std::memory_order_release);
        co_return;
    });
    auto subscription = client.subscribe<HttpEvent>([&](const HttpEvent& event) -> Task<void> {
        received.store(event.value, std::memory_order_release);
        co_return;
    });

    client.publish(HttpEvent{0x12345678});
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while ((received.load(std::memory_order_acquire) == 0 || received_by_server.load(std::memory_order_acquire) == 0) &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(received.load(std::memory_order_acquire) == 0x12345678);
    REQUIRE(received_by_server.load(std::memory_order_acquire) == 0x12345678);
    client.drain().wait();
    server.drain().wait();
}
#endif
