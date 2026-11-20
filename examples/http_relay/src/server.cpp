#include <aw/tobus/tobus.hpp>
#include <aw/tobus/transports/http_bus.hpp>

#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char** argv) {
    aw::tobus::EventBusConfig config;
    config.transport = aw::tobus::TransportKind::http;
    config.endpoint = argc > 1 ? argv[1] : "0.0.0.0:8080";
    config.options["role"] = "server";

    auto bus = aw::tobus::EventBusFactory::create(config);
    std::cout << "HTTP relay listening on " << config.endpoint << "; press Ctrl-C to stop\n" << std::flush;
    for (;;) std::this_thread::sleep_for(std::chrono::hours(1));
}
