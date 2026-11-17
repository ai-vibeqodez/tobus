#pragma once

/**
 * @file rabbitmq_bus.hpp
 * @brief @c RabbitMQBus: RabbitMQ (AMQP). Needs a client library (e.g. rabbitmq-c) when implemented. STUB: declarations only, every body is a TODO.
 */

#include <aw/tobus/errors.hpp>
#include <aw/tobus/transports/framed_bus.hpp>

#include <utility>

namespace aw::tobus {

/**
 * @class RabbitMQBus
 * @brief Not implemented yet. Implement it using @c InProcessBus (delivery) and @c FramedBus (framing) as reference.
 *        Enabled with the CMake option @c TOBUS_ENABLE_RABBITMQ.
 */
class RabbitMQBus final : public FramedBus {
public:
    /// @throws TransportNotImplemented Always (until implemented).
    explicit RabbitMQBus(EventBusConfig config);
    ~RabbitMQBus() override;

    [[nodiscard]] std::string_view transport_name() const noexcept override;

protected:
    void send_frame(const Frame& frame) override;
};

inline RabbitMQBus::RabbitMQBus(EventBusConfig config)
    : FramedBus(std::move(config)) {
    // TODO: implement (connect / bind using config().endpoint and config().options)
    throw TransportNotImplemented("RabbitMQBus");
}

inline RabbitMQBus::~RabbitMQBus() = default;

inline std::string_view RabbitMQBus::transport_name() const noexcept {
    return "rabbitmq";
}

inline void RabbitMQBus::send_frame(const Frame&) {
    // TODO: implement
    throw TransportNotImplemented("RabbitMQBus::send_frame");
}

} // namespace aw::tobus
