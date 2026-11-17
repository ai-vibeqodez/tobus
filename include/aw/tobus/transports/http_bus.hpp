#pragma once

/**
 * @file http_bus.hpp
 * @brief @c HTTPBus: HTTP POST publish / long-poll or SSE subscribe. STUB: declarations only, every body is a TODO.
 */

#include <aw/tobus/transports/framed_bus.hpp>

namespace aw::tobus {

/**
 * @class HTTPBus
 * @brief Not implemented yet. Implement it using @c InProcessBus (delivery) and @c FramedBus (framing) as reference.
 *        Enabled with the CMake option @c TOBUS_ENABLE_HTTP.
 */
class HTTPBus final : public FramedBus {
public:
    /// @throws TransportNotImplemented Always (until implemented).
    explicit HTTPBus(EventBusConfig config);
    ~HTTPBus() override;

    [[nodiscard]] std::string_view transport_name() const noexcept override;

protected:
    void send_frame(const Frame& frame) override;
};

} // namespace aw::tobus
