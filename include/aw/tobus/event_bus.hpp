#ifndef AW_TOBUS_EVENT_BUS_HPP
#define AW_TOBUS_EVENT_BUS_HPP

/**
 * @file event_bus.hpp
 * @brief @c IEventBus: the only type modules depend on.
 */

#include <aw/tobus/event_handler.hpp>
#include <aw/tobus/event_traits.hpp>
#include <aw/tobus/event_type_id.hpp>
#include <aw/tobus/subscription.hpp>

#include <concepts>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace aw::tobus {

/**
 * @class IEventBus
 * @brief Transport-agnostic, type-safe event bus (hybrid facade).
 *
 * Template members cannot be virtual, so the typed API (@c publish<T>, @c subscribe<T>) is non-virtual and
 * forwards to two protected pure virtuals that speak in type-erased terms (@c EventTypeInfo + a
 * @c shared_ptr<const void>). A transport implements those two plus @c drain() and @c transport_name().
 *
 * Semantics:
 *  - @c publish is fire-and-forget: it never waits for handlers; handlers run concurrently on the @c aw::tpl pool.
 *  - No ordering guarantee between handlers, or between two events delivered to one handler.
 *  - A handler's exceptions go to @c EventBusConfig::on_error (stderr if none is set), never to the publisher.
 *
 * Lifecycle: there is no @c shutdown(). @c drain() is the quiescence point, and destroying the bus never blocks
 * and never invalidates running handlers. Transports that own connections close them in their destructor;
 * if they need an asynchronous close, it will be added as a non-pure virtual with a @c drain()-based default,
 * which does not break existing transports or callers.
 */
class IEventBus {
public:
    IEventBus(const IEventBus&) = delete;
    IEventBus& operator=(const IEventBus&) = delete;
    virtual ~IEventBus() = default;

    /**
     * @brief Publishes @p event to every subscriber of @c T and returns immediately.
     *
     * The event is moved/copied once into shared storage; subscribers receive a reference to that one object.
     * @tparam E Deduced; the event type is @c std::remove_cvref_t<E>.
     * @throws std::logic_error If a different event type with the same @c EventTypeId has subscribers on this bus
     *         (two types declare the same @c EventTraits name). Delivering would reinterpret one type as the other.
     */
    template <typename E>
        requires std::constructible_from<std::remove_cvref_t<E>, E&&>
    void publish(E&& event) {
        using T = std::remove_cvref_t<E>;
        publish_raw(event_type_info<T>(), std::make_shared<const T>(std::forward<E>(event)));
    }

    /**
     * @brief Registers @p handler for events of type @p T.
     * @tparam T Event type (spelled explicitly: @c bus.subscribe<Ping>(...)).
     * @return Handle that unsubscribes on destruction; discarding it unsubscribes immediately.
     * @throws std::invalid_argument If @p handler is empty.
     * @throws NotSerializable From network transports if @p T cannot be serialized.
     */
    template <typename T>
    [[nodiscard]] Subscription subscribe(EventHandler<T> handler) {
        static_assert(std::is_same_v<T, std::remove_cvref_t<T>>, "event types must be unqualified");
        if (!handler) {
            throw std::invalid_argument("IEventBus::subscribe: empty handler");
        }
        return subscribe_raw(event_type_info<T>(),
                             [h = std::move(handler)](const void* event) { return h(*static_cast<const T*>(event)); });
    }

    /**
     * @brief Completes when every delivery started so far (including deliveries triggered by handlers
     *        publishing further events) has finished. A quiescence point for shutdown and tests;
     *        publishing is not blocked meanwhile.
     */
    [[nodiscard]] virtual Task<void> drain() = 0;

    /// Name of the transport, as in @c to_string(TransportKind).
    [[nodiscard]] virtual std::string_view transport_name() const noexcept = 0;

protected:
    IEventBus() = default;

    /// Type-erased publish. @p event points at an object of the type described by @p type.
    virtual void publish_raw(const EventTypeInfo& type, std::shared_ptr<const void> event) = 0;

    /// Type-erased subscribe; @p handler receives a pointer to an object of the type described by @p type.
    virtual Subscription subscribe_raw(const EventTypeInfo& type, RawEventHandler handler) = 0;
};

} // namespace aw::tobus

#endif  // AW_TOBUS_EVENT_BUS_HPP
