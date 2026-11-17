#pragma once

/**
 * @file dispatcher.hpp
 * @brief Internal: the delivery engine shared by every transport (subscription table + concurrent handler spawning).
 */

#include <aw/tobus/event_handler.hpp>
#include <aw/tobus/event_type_id.hpp>
#include <aw/tobus/subscription.hpp>

#include <aw/tpl/threading/tasks/task_group.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace aw::tobus::detail {

/**
 * @class Dispatcher
 * @brief Delivers an event (already in memory) to the handlers registered for its type.
 *
 * The subscription table is copy-on-write: @c dispatch only copies a @c shared_ptr under a short lock and then
 * iterates an immutable snapshot, so handlers may subscribe/unsubscribe/publish freely.
 * Every delivery is one eager @c aw::tpl task tracked by a @c TaskGroup; a task owns a reference to the
 * dispatcher, so in-flight deliveries stay valid even if the bus is destroyed first.
 */
class Dispatcher : public std::enable_shared_from_this<Dispatcher> {
    struct PrivateTag {};

public:
    explicit Dispatcher(PrivateTag, ErrorHandler on_error);

    /// Dispatchers are shared (deliveries and subscriptions refer back to them), hence a factory.
    [[nodiscard]] static std::shared_ptr<Dispatcher> create(ErrorHandler on_error);

    /**
     * @brief Registers @p handler for @p type.
     * @throws std::logic_error If another event type already uses the same @c EventTypeId.
     */
    [[nodiscard]] Subscription subscribe(const EventTypeInfo& type, RawEventHandler handler);

    /// Starts one concurrent delivery per handler of @p type and returns without waiting.
    void dispatch(const EventTypeInfo& type, std::shared_ptr<const void> event);

    /// Completes when no delivery is in flight (see @c IEventBus::drain). Keeps @p self alive meanwhile.
    [[nodiscard]] static Task<void> drain(std::shared_ptr<Dispatcher> self);

    /// Forwards @p ep to the configured error handler; never throws.
    void report(std::exception_ptr ep) noexcept;

    /// Number of registered handlers for @p id.
    [[nodiscard]] std::size_t subscriber_count(EventTypeId id) const;

private:
    struct Entry {
        std::uint64_t token;
        std::shared_ptr<RawEventHandler> handler;
    };
    using Table = std::unordered_map<EventTypeId, std::vector<Entry>>;

    void unsubscribe(EventTypeId id, std::uint64_t token) noexcept;
    [[nodiscard]] static Task<void> run(std::shared_ptr<Dispatcher> self, std::shared_ptr<RawEventHandler> handler,
                                        std::shared_ptr<const void> event);

    ErrorHandler on_error_;
    mutable std::mutex mtx_;
    std::shared_ptr<const Table> table_ = std::make_shared<const Table>();
    std::unordered_map<EventTypeId, const std::type_info*> identities_;
    std::uint64_t next_token_ = 1;
    aw::tpl::threading::tasks::TaskGroup inflight_;
};

} // namespace aw::tobus::detail
