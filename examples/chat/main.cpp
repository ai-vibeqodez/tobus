// Chat example — composition root.
//
// This is the only file that knows which transport is used. Change `config.transport`
// to move the same modules onto another one (a socket, a broker); the modules below stay untouched.

#include <aw/tobus/tobus.hpp>
#include <aw/tpl/threading/tasks/async_sleep.hpp>

#include <chrono>
#include <iostream>
#include <mutex>
#include <string>

#include "auth.hpp"
#include "moderation.hpp"
#include "notifier.hpp"
#include "room.hpp"
#include "stats.hpp"

using namespace aw::tobus;
using namespace aw::tobus::examples::chat;
using namespace std::chrono_literals;

// ---- helpers ----------------------------------------------------------------

// Serialize console output (handlers run on pool threads).
static void log(std::string_view line) {
    static std::mutex m;
    std::lock_guard lock(m);
    std::cout << line << "\n";
}

// ---- main -------------------------------------------------------------------

int main() {
    EventBusConfig config;
    config.transport = TransportKind::in_process;
    // An exception thrown in any handler ends up here, never at the publisher.
    config.on_error = [](const std::exception_ptr& ep) {
        try {
            std::rethrow_exception(ep);
        } catch (const std::exception& ex) {
            log(std::string("[error]     handler failed: ") + ex.what());
        }
    };

    auto bus = EventBusFactory::create(config);
    log("transport: " + std::string(bus->transport_name()));

    // Compose modules — each one only knows IEventBus&.
    Auth auth(*bus);
    Room room(*bus, log);
    Moderation moderation(*bus);
    Notifier notifier(*bus, log);
    Stats stats(*bus);

    // Two "users" act. Every call returns immediately (fire-and-forget):
    // nobody waits for moderation, the room, or the stats. They happen in
    // the background, on the thread pool.
    auth.login("alice");
    auth.login("bob");
    room.say("alice", "hello everyone!");
    room.say("bob", "buy cheap spam now");
    room.say("alice", "welcome, bob");

    // drain() completes once every started delivery has finished, including
    // the ones triggered by other handlers (Moderation's cascade).
    // Use it at shutdown or in tests; normal operation never needs to wait.
    bus->drain().wait();

    std::cout << "\njoined=" << stats.joined << " sent=" << stats.sent
              << " approved=" << stats.approved << " blocked=" << stats.blocked
              << "\n";

    return 0;
}
