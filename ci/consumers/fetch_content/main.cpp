// Consumer integration test: FetchContent pattern
// Tests that tobus can be consumed via FetchContent_MakeAvailable(tobus)

#include <aw/tobus/tobus.hpp>
#include <iostream>

int main() {
    // Minimal smoke test: create event bus and verify basic functionality
    auto config = aw::tobus::EventBusConfig{};
    auto bus = aw::tobus::EventBusFactory::create(config);
    
    if (!bus) {
        std::cerr << "Failed to create event bus\n";
        return 1;
    }
    
    std::cout << "FetchContent consumer: OK\n";
    return 0;
}
