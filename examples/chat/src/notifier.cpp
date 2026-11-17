// Notifier implementation.

#include "notifier.hpp"

namespace aw::tobus::examples::chat {

Notifier::Notifier(IEventBus& bus, void (*log)(std::string_view))
    : sub_(bus.subscribe<MessageBlocked>([log](const MessageBlocked& e) -> Task<void> {
          log("[notifier]  @" + e.from + ": your message was blocked (" + e.reason + ")");
          co_return;
      })) {}

} // namespace aw::tobus::examples::chat
