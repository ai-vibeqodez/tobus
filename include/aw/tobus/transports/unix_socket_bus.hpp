#pragma once

/**
 * @file unix_socket_bus.hpp
 * @brief @c UnixSocketBus: Unix domain socket (POSIX sockets, no extra dependency). STUB: declarations only, every body is a TODO.
 */

#include <aw/tobus/transports/framed_bus.hpp>

namespace aw::tobus {

/**
 * @class UnixSocketBus
 * @brief Not implemented yet. Implement it using @c InProcessBus (delivery) and @c FramedBus (framing) as reference.
 *        Enabled with the CMake option @c TOBUS_ENABLE_UNIX_SOCKET.
 */
class UnixSocketBus final : public FramedBus {
public:
    /// @throws TransportNotImplemented Always (until implemented).
    explicit UnixSocketBus(EventBusConfig config);
    ~UnixSocketBus() override;

    [[nodiscard]] std::string_view transport_name() const noexcept override;

protected:
    void send_frame(const Frame& frame) override;
};

} // namespace aw::tobus
