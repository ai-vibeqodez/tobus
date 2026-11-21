#include <http_relay/events.hpp>

#include <aw/tobus/transports/http_bus.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char** argv) {
    aw::tobus::EventBusConfig config;
    config.transport = aw::tobus::TransportKind::http;
    config.endpoint = argc > 1 ? argv[1] : "http://127.0.0.1:8080";
    config.options["role"] = "client";
    auto bus = aw::tobus::EventBusFactory::create(config);

    std::atomic<std::uint32_t> readings{0};
    auto subscription = bus->subscribe<http_relay::TemperatureReading>(
        [&readings](const http_relay::TemperatureReading& event) -> aw::tobus::Task<void> {
            readings.fetch_add(1, std::memory_order_relaxed);
            const auto fahrenheit = event.celsius * 9 / 5 + 32;
            std::cout << "received reading #" << event.sequence << ": " << event.celsius << " C, converted to "
                      << fahrenheit << " F\n"
                      << std::flush;
            co_return;
        });

    std::cout << "Listening for temperature readings; press Ctrl-C to stop\n" << std::flush;
    while (subscription.active())
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
}
