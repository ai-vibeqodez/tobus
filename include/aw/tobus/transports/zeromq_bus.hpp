#pragma once

/**
 * @file zeromq_bus.hpp
 * @brief @c ZeroMQBus: ZeroMQ PUB/SUB. Needs libzmq when implemented. STUB: declarations only, every body is a TODO.
 */

#include <aw/tobus/errors.hpp>
#include <aw/tobus/transports/framed_bus.hpp>

#include <utility>

namespace aw::tobus {

/**
 * @class ZeroMQBus
 * @brief Not implemented yet. Implement it using @c InProcessBus (delivery) and @c FramedBus (framing) as reference.
 *        Enabled with the CMake option @c TOBUS_ENABLE_ZEROMQ.
 */
class ZeroMQBus final : public FramedBus {
public:
    /// @throws TransportNotImplemented Always (until implemented).
    explicit ZeroMQBus(EventBusConfig config);
    ~ZeroMQBus() override;

    [[nodiscard]] std::string_view transport_name() const noexcept override;

protected:
    void send_frame(const Frame& frame) override;
};

inline ZeroMQBus::ZeroMQBus(EventBusConfig config)
    : FramedBus(std::move(config)) {
    // TODO: implement (connect / bind using config().endpoint and config().options)
    throw TransportNotImplemented("ZeroMQBus");
}

inline ZeroMQBus::~ZeroMQBus() = default;

inline std::string_view ZeroMQBus::transport_name() const noexcept {
    return "zeromq";
}

inline void ZeroMQBus::send_frame(const Frame&) {
    // TODO: implement
    throw TransportNotImplemented("ZeroMQBus::send_frame");
}

} // namespace aw::tobus
