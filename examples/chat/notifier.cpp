// Notifier implementation.

#include "notifier.hpp"
#include <iostream>
#include <mutex>

namespace aw::tobus::examples::chat {

static void log(std::string_view line) {
    static std::mutex m;
    std::lock_guard lock(m);
    std::cout << line << "\n";
}

Notifier::Notifier(IEventBus& bus, void (*log)(std::string_view))
    : sub_(bus.subscribe<MessageBlocked>([log](const MessageBlocked& e) -> Task<void> {
          log("[notifier]  @" + e.from + ": your message was blocked (" + e.reason + ")");
          co_return;
      })) {}

} // namespace aw::tobus::examples::chat
