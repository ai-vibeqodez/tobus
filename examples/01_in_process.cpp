// InProcessBus demo: modules only know IEventBus; the transport is chosen by configuration.
#include <aw/tobus/tobus.hpp>

#include <atomic>
#include <iostream>
#include <string>

using namespace aw::tobus;

struct UserJoined {
    std::string name;
};

struct MessageSent {
    std::string from;
    std::string text;
};

// A "module": publishes and subscribes through IEventBus only.
class ChatModule {
public:
    explicit ChatModule(IEventBus& bus)
        : bus_(bus),
          joined_(bus.subscribe<UserJoined>([this](const UserJoined& e) -> Task<void> {
              ++joins_;
              std::cout << "[chat] welcome, " << e.name << "\n";
              co_return;
          })) {}

    void say(std::string from, std::string text) { bus_.publish(MessageSent{std::move(from), std::move(text)}); }

    [[nodiscard]] int joins() const { return joins_; }

private:
    IEventBus& bus_;
    std::atomic<int> joins_{0};
    Subscription joined_;
};

int main() {
    EventBusConfig config;
    config.transport = TransportKind::in_process; // change this (and nothing else) to switch transport
    config.on_error = [](std::exception_ptr ep) {
        try {
            std::rethrow_exception(ep);
        } catch (const std::exception& ex) {
            std::cerr << "handler failed: " << ex.what() << "\n";
        }
    };
    auto bus = EventBusFactory::create(config);
    std::cout << "transport: " << bus->transport_name() << "\n";

    ChatModule chat(*bus);

    // Handlers are coroutines; the event reference stays valid until the returned task completes.
    auto logger = bus->subscribe<MessageSent>([](const MessageSent& e) -> Task<void> {
        std::cout << "[log] " << e.from << ": " << e.text << "\n";
        co_return;
    });

    bus->publish(UserJoined{"alice"}); // fire-and-forget: returns before any handler ran
    chat.say("alice", "hello!");

    bus->drain().wait(); // quiescence point (shutdown / tests); not needed for normal operation
    std::cout << "joins: " << chat.joins() << "\n";
    return 0;
}
