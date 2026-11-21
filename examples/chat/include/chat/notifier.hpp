// Notifier module — tells the author their message was blocked.
// Knows nothing about Moderation, only about MessageBlocked.

#ifndef AW_TOBUS_CHAT_NOTIFIER_HPP
#define AW_TOBUS_CHAT_NOTIFIER_HPP

#include <aw/tobus/tobus.hpp>
#include "events.hpp"

using namespace aw::tobus;

namespace aw::tobus::examples::chat {

class Notifier {
public:
    explicit Notifier(IEventBus& bus, void (*log)(std::string_view));

private:
    Subscription sub_;
};

} // namespace aw::tobus::examples::chat

#endif  // AW_TOBUS_CHAT_NOTIFIER_HPP
