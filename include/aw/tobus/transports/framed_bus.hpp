#pragma once

/**
 * @file framed_bus.hpp
 * @brief @c FramedBus: shared implementation for transports that move serialized frames.
 *
 *  - @c publish_raw: deliver locally through the shared dispatcher, then encode and send serializable events.
 *  - @c subscribe_raw: require a codec, register the handler and remember the type decoder.
 *  - @c receive_frame: drop frames originating at this bus, otherwise decode and dispatch.
 */

#include <aw/tobus/event_bus.hpp>
#include <aw/tobus/event_bus_config.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace aw::tobus::detail {
class Dispatcher;
}

namespace aw::tobus {

/**
 * @class FramedBus
 * @brief Base of transports that move opaque frames: a derived class only implements "send a frame" and calls
 *        @c receive_frame() for each incoming one.
 */
class FramedBus : public IEventBus {
public:
    /// One serialized event on the wire.
    struct Frame {
        std::uint64_t origin = 0; ///< @c origin_id() of the publishing bus; used to drop our own echo.
        EventTypeId type;         ///< Which event type @c payload encodes.
        std::vector<std::byte> payload;
    };

    explicit FramedBus(EventBusConfig config);
    ~FramedBus() override;

    /// Random id of this bus instance, stamped into every outgoing frame.
    [[nodiscard]] std::uint64_t origin_id() const noexcept { return origin_id_; }

    /// Completes when all local handler deliveries started so far have finished.
    [[nodiscard]] Task<void> drain() override;

protected:
    /// Delivers locally and sends a serialized frame when the event has a codec.
    void publish_raw(const EventTypeInfo& type, std::shared_ptr<const void> event) override;

    /// Requires a codec and registers the handler for matching received frames.
    Subscription subscribe_raw(const EventTypeInfo& type, RawEventHandler handler) override;

    /// Transports implement this: write @p frame to the wire (must not block the caller).
    virtual void send_frame(const Frame& frame) = 0;

    /// Decodes and dispatches a received frame, dropping this bus's own echo.
    void receive_frame(const Frame& frame);

    /// Reports an asynchronous transport failure through the configured error handler.
    void report_transport_error(const std::exception_ptr& error) noexcept;

    /// Throws @c NotSerializable unless @p type is named and has a codec.
    static void require_serializable(const EventTypeInfo& type);

    [[nodiscard]] const EventBusConfig& config() const noexcept { return config_; }

private:
    EventBusConfig config_;
    std::uint64_t origin_id_;
    std::shared_ptr<detail::Dispatcher> dispatcher_;
    mutable std::mutex decoders_mtx_;
    std::unordered_map<EventTypeId, EventTypeInfo> event_types_;
};

} // namespace aw::tobus
