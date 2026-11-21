#ifndef AW_TOBUS_TRANSPORTS_WEBSOCKET_BUS_HPP
#define AW_TOBUS_TRANSPORTS_WEBSOCKET_BUS_HPP

/**
 * @file websocket_bus.hpp
 * @brief @c WebSocketBus: WebSocket (RFC 6455) client/server. STUB: declarations only, every body is a TODO.
 */

#include <aw/tobus/transports/framed_bus.hpp>

namespace aw::tobus {

/**
 * @class WebSocketBus
 * @brief Not implemented yet. Implement it using @c InProcessBus (delivery) and @c FramedBus (framing) as reference.
 *        Enabled with the CMake option @c TOBUS_ENABLE_WEBSOCKET.
 */
class WebSocketBus final : public FramedBus {
public:
    /// @throws TransportNotImplemented Always (until implemented).
    explicit WebSocketBus(EventBusConfig config);
    ~WebSocketBus() override;

    [[nodiscard]] std::string_view transport_name() const noexcept override;

protected:
    void send_frame(const Frame& frame) override;
};

} // namespace aw::tobus

#endif  // AW_TOBUS_TRANSPORTS_WEBSOCKET_BUS_HPP
