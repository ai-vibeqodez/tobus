// Auth module — publishes UserJoined. Does not know who (if anyone) cares.

#pragma once

#include <aw/tobus/tobus.hpp>
#include "events.hpp"

using namespace aw::tobus;

namespace aw::tobus::examples::chat {

class Auth {
public:
    explicit Auth(IEventBus& bus)
        : bus_(bus) {}

    void login(std::string name) {
        bus_.publish(UserJoined{std::move(name)});
    }

private:
    IEventBus& bus_;
};

} // namespace aw::tobus::examples::chat
