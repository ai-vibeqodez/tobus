#pragma once

/**
 * @file framed_bus.hpp
 * @brief @c FramedBus: foundation for network transports (STUB). Not part of the MVP implementation.
 *
 * Contract for the future implementation (see @c InProcessBus as the reference):
 *  - @c publish_raw: deliver to local subscribers directly through the shared dispatcher (no serialization),
 *    then, if the type is serializable, encode it into a @c Frame stamped with @c origin_id() and @c send_frame() it.
 *    A non-serializable type is reported to @c EventBusConfig::on_error (safety net; @c subscribe already rejects it).
 *  - @c subscribe_raw: throw @c NotSerializable if the type cannot be encoded (surfaces at startup), else register
 *    the handler and remember the type's @c decode so received frames can be turned back into events.
 *  - @c receive_frame: drop frames whose origin is this bus (own echo), otherwise decode and dispatch.
 */

#include <aw/tobus/event_bus.hpp>
#include <aw/tobus/event_bus_config.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
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

    /// TODO: implement (await the shared dispatcher, like InProcessBus).
    [[nodiscard]] Task<void> drain() override;

protected:
    /// TODO: implement as described in the file comment.
    void publish_raw(const EventTypeInfo& type, std::shared_ptr<const void> event) override;

    /// TODO: implement as described in the file comment.
    Subscription subscribe_raw(const EventTypeInfo& type, RawEventHandler handler) override;

    /// Transports implement this: write @p frame to the wire (must not block the caller).
    virtual void send_frame(const Frame& frame) = 0;

    /// TODO: implement: transports call this for every received frame.
    void receive_frame(const Frame& frame);

    /// Throws @c NotSerializable unless @p type is named and has a codec.
    static void require_serializable(const EventTypeInfo& type);

    [[nodiscard]] const EventBusConfig& config() const noexcept { return config_; }

private:
    EventBusConfig config_;
    std::uint64_t origin_id_;
    // TODO: std::shared_ptr<detail::Dispatcher> dispatcher_; map EventTypeId -> EventTypeInfo::decode.
};

} // namespace aw::tobus
