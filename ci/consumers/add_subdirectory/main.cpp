// Consumer integration test: add_subdirectory pattern
// Tests that tobus can be consumed via add_subdirectory(tobus)

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
    
    std::cout << "add_subdirectory consumer: OK\n";
    return 0;
}
