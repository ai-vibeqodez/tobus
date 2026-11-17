// A small chat service built from independent modules that know nothing about each other.
//
// The only thing they share is the bus (IEventBus) and the event types. Nobody calls anybody:
//
//   Auth::login()  --UserJoined-->   Room (prints "joined"), Stats
//   Room::say()    --MessageSent-->  Moderation, Stats
//   Moderation     --MessageApproved--> Room (prints the message), Stats
//                  --MessageBlocked-->  Notifier (warns the author), Stats
//
// Moderation reacts to one event by publishing another one: a cascade. Every handler is a coroutine started
// concurrently per delivery, so the order of the log lines below may differ from run to run; that is expected.
#include <aw/tobus/tobus.hpp>
#include <aw/tpl/threading/tasks/async_sleep.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>

using namespace aw::tobus;
using namespace std::chrono_literals;

// ---- events: plain structs, this is the whole "contract" between modules --------------------------------------------

struct UserJoined {
    std::string name;
};

struct MessageSent { // raw input from a user, not yet checked
    std::string from;
    std::string text;
};

struct MessageApproved { // moderation said "ok": safe to show to everybody
    std::string from;
    std::string text;
};

struct MessageBlocked { // moderation said "no"
    std::string from;
    std::string reason;
};

// Handlers run on pool threads, so serialize console output (otherwise lines interleave mid-line).
void log(std::string_view line) {
    static std::mutex m;
    std::lock_guard lock(m);
    std::cout << line << "\n";
}

// ---- modules: each one publishes and/or subscribes through IEventBus only -------------------------------------------

// Publishes UserJoined. Does not know who (if anyone) cares.
class Auth {
public:
    explicit Auth(IEventBus& bus)
        : bus_(bus) {}
    void login(std::string name) { bus_.publish(UserJoined{std::move(name)}); }

private:
    IEventBus& bus_;
};

// The chat room: users talk into it, and it shows whatever moderation approved.
class Room {
public:
    explicit Room(IEventBus& bus)
        : bus_(bus),
          joined_(bus.subscribe<UserJoined>([](const UserJoined& e) -> Task<void> {
              log("[room]      * " + e.name + " joined");
              co_return;
          })),
          approved_(bus.subscribe<MessageApproved>([](const MessageApproved& e) -> Task<void> {
              log("[room]      " + e.from + ": " + e.text);
              co_return;
          })) {}

    // Publishes the *raw* message. The room never shows it directly: only MessageApproved reaches the screen.
    void say(std::string from, std::string text) { bus_.publish(MessageSent{std::move(from), std::move(text)}); }

private:
    IEventBus& bus_;
    // A Subscription is an RAII ticket: destroying it unsubscribes the handler.
    Subscription joined_;
    Subscription approved_;
};

// Checks every MessageSent and answers with MessageApproved or MessageBlocked.
class Moderation {
public:
    explicit Moderation(IEventBus& bus)
        : bus_(bus),
          sub_(bus.subscribe<MessageSent>([this](const MessageSent& e) -> Task<void> {
              // Pretend the check is slow (a remote spam filter, say). co_await suspends only this handler;
              // the publisher and every other handler keep running.
              co_await aw::tpl::threading::tasks::async_sleep(20ms);

              if (e.text.find("spam") != std::string::npos) {
                  bus_.publish(MessageBlocked{e.from, "looks like spam"}); // cascade: a handler publishes
              } else {
                  bus_.publish(MessageApproved{e.from, e.text});
              }
          })) {}

private:
    IEventBus& bus_;
    Subscription sub_;
};

// Tells the author their message did not go through. Knows nothing about Moderation, only about MessageBlocked.
class Notifier {
public:
    explicit Notifier(IEventBus& bus)
        : sub_(bus.subscribe<MessageBlocked>([](const MessageBlocked& e) -> Task<void> {
              log("[notifier]  @" + e.from + ": your message was blocked (" + e.reason + ")");
              co_return;
          })) {}

private:
    Subscription sub_;
};

// Pure observer: subscribes to everything, publishes nothing. Adding it required no change in any other module.
class Stats {
public:
    explicit Stats(IEventBus& bus)
        : joined_(bus.subscribe<UserJoined>([this](const UserJoined&) -> Task<void> {
              ++joined;
              co_return;
          })),
          sent_(bus.subscribe<MessageSent>([this](const MessageSent&) -> Task<void> {
              ++sent;
              co_return;
          })),
          approved_(bus.subscribe<MessageApproved>([this](const MessageApproved&) -> Task<void> {
              ++approved;
              co_return;
          })),
          blocked_(bus.subscribe<MessageBlocked>([this](const MessageBlocked&) -> Task<void> {
              ++blocked;
              co_return;
          })) {}

    // Handlers run concurrently on different threads, hence atomics.
    std::atomic<int> joined{0}, sent{0}, approved{0}, blocked{0};

private:
    Subscription joined_, sent_, approved_, blocked_;
};

int main() {
    // Composition root: the only place that knows which transport is used. Change `transport` to move the same
    // modules onto another one (a socket, a broker); the modules below stay untouched.
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

    Auth auth(*bus);
    Room room(*bus);
    Moderation moderation(*bus);
    Notifier notifier(*bus);
    Stats stats(*bus);

    // Two "users" act. Every call returns immediately (fire-and-forget): nobody waits for moderation, the room
    // or the stats. They happen in the background, on the thread pool.
    auth.login("alice");
    auth.login("bob");
    room.say("alice", "hello everyone!");
    room.say("bob", "buy cheap spam now");
    room.say("alice", "welcome, bob");

    // The handlers are still running in the background at this point. drain() completes once every started
    // delivery has finished, including the ones triggered by other handlers (Moderation's cascade).
    // Use it at shutdown or in tests; normal operation never needs to wait.
    bus->drain().wait();

    std::cout << "\njoined=" << stats.joined << " sent=" << stats.sent << " approved=" << stats.approved
              << " blocked=" << stats.blocked << "\n";
    return 0;
}
