#include <http_relay/events.hpp>

#include <aw/tobus/transports/http_bus.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char** argv) {
    aw::tobus::EventBusConfig config;
    config.transport = aw::tobus::TransportKind::http;
    config.endpoint = argc > 1 ? argv[1] : "http://127.0.0.1:8080";
    config.options["role"] = "client";
    auto bus = aw::tobus::EventBusFactory::create(config);

    for (std::uint32_t sequence = 1; sequence <= 5; ++sequence) {
        const auto celsius = static_cast<std::int32_t>(20 + sequence);
        bus->publish(http_relay::TemperatureReading{sequence, celsius});
        std::cout << "published reading #" << sequence << ": " << celsius << " C\n" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}
