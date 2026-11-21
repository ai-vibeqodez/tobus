// Room module — users talk into it; it shows only what moderation approved.

#ifndef AW_TOBUS_CHAT_ROOM_HPP
#define AW_TOBUS_CHAT_ROOM_HPP

#include <aw/tobus/tobus.hpp>
#include "events.hpp"

using namespace aw::tobus;

namespace aw::tobus::examples::chat {

class Room {
public:
    explicit Room(IEventBus& bus, void (*log)(std::string_view));

    // Publishes the *raw* message. The room never shows it directly:
    // only MessageApproved reaches the screen.
    void say(std::string from, std::string text);

private:
    IEventBus& bus_;
    void (*log_)(std::string_view);
    Subscription joined_;
    Subscription approved_;
};

} // namespace aw::tobus::examples::chat

#endif  // AW_TOBUS_CHAT_ROOM_HPP
