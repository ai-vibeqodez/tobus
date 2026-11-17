#pragma once

/**
 * @file tcp_server_bus.hpp
 * @brief @c TCPServerBus: TCP server/clients with length-prefixed frames. STUB: declarations only, every body is a TODO.
 */

#include <aw/tobus/transports/framed_bus.hpp>

namespace aw::tobus {

/**
 * @class TCPServerBus
 * @brief Not implemented yet. Implement it using @c InProcessBus (delivery) and @c FramedBus (framing) as reference.
 *        Enabled with the CMake option @c TOBUS_ENABLE_TCP_SERVER.
 */
class TCPServerBus final : public FramedBus {
public:
    /// @throws TransportNotImplemented Always (until implemented).
    explicit TCPServerBus(EventBusConfig config);
    ~TCPServerBus() override;

    [[nodiscard]] std::string_view transport_name() const noexcept override;

protected:
    void send_frame(const Frame& frame) override;
};

} // namespace aw::tobus
