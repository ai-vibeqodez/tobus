// Stats implementation.

#include "stats.hpp"

namespace aw::tobus::examples::chat {

Stats::Stats(IEventBus& bus)
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

} // namespace aw::tobus::examples::chat
