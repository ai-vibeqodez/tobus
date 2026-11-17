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
#include <string>
#include <string_view>
#include <typeinfo>
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

    /**
     * @brief Starts one concurrent delivery per handler of @p type and returns without waiting.
     * @throws std::logic_error If another event type with the same @c EventTypeId has subscribers here.
     */
    void dispatch(const EventTypeInfo& type, const std::shared_ptr<const void>& event);

    /// Completes when no delivery is in flight (see @c IEventBus::drain). Keeps @p self alive meanwhile.
    [[nodiscard]] static Task<void> drain(std::shared_ptr<Dispatcher> self);

    /**
     * @brief Forwards @p ep to the configured error handler; never throws.
     *
     * Without a configured handler the failure is written to stderr (naming @p event_name) instead of being lost.
     */
    void report(const std::exception_ptr& ep, std::string_view event_name) noexcept;

    /// Number of registered handlers for @p id.
    [[nodiscard]] std::size_t subscriber_count(EventTypeId id) const;

private:
    struct Entry {
        std::uint64_t token;
        std::shared_ptr<RawEventHandler> handler;
    };
    using Table = std::unordered_map<EventTypeId, std::vector<Entry>>;

    void unsubscribe(EventTypeId id, std::uint64_t token) noexcept;
    [[nodiscard]] static std::string collision_message(std::string_view event_name);
    [[nodiscard]] static Task<void> run(std::shared_ptr<Dispatcher> self, std::shared_ptr<RawEventHandler> handler,
                                        std::shared_ptr<const void> event, std::string_view event_name);

    ErrorHandler on_error_;
    mutable std::mutex mtx_;
    std::shared_ptr<const Table> table_ = std::make_shared<const Table>();
    std::unordered_map<EventTypeId, const std::type_info*> identities_;
    std::uint64_t next_token_ = 1;
    aw::tpl::threading::tasks::TaskGroup inflight_;
};

} // namespace aw::tobus::detail
