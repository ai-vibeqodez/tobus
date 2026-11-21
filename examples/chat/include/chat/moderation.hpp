// Moderation module — checks every MessageSent and answers with MessageApproved or MessageBlocked.

#ifndef AW_TOBUS_CHAT_MODERATION_HPP
#define AW_TOBUS_CHAT_MODERATION_HPP

#include <aw/tobus/tobus.hpp>
#include "events.hpp"

using namespace aw::tobus;

namespace aw::tobus::examples::chat {

class Moderation {
public:
    explicit Moderation(IEventBus& bus);

private:
    IEventBus& bus_;
    Subscription sub_;
};

} // namespace aw::tobus::examples::chat

#endif  // AW_TOBUS_CHAT_MODERATION_HPP
