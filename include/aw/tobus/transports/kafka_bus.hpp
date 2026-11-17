#pragma once

/**
 * @file kafka_bus.hpp
 * @brief @c KafkaBus: Kafka producer/consumer. Needs librdkafka when implemented. STUB: declarations only, every body is a TODO.
 */

#include <aw/tobus/errors.hpp>
#include <aw/tobus/transports/framed_bus.hpp>

#include <utility>

namespace aw::tobus {

/**
 * @class KafkaBus
 * @brief Not implemented yet. Implement it using @c InProcessBus (delivery) and @c FramedBus (framing) as reference.
 *        Enabled with the CMake option @c TOBUS_ENABLE_KAFKA.
 */
class KafkaBus final : public FramedBus {
public:
    /// @throws TransportNotImplemented Always (until implemented).
    explicit KafkaBus(EventBusConfig config);
    ~KafkaBus() override;

    [[nodiscard]] std::string_view transport_name() const noexcept override;

protected:
    void send_frame(const Frame& frame) override;
};

inline KafkaBus::KafkaBus(EventBusConfig config)
    : FramedBus(std::move(config)) {
    // TODO: implement (connect / bind using config().endpoint and config().options)
    throw TransportNotImplemented("KafkaBus");
}

inline KafkaBus::~KafkaBus() = default;

inline std::string_view KafkaBus::transport_name() const noexcept {
    return "kafka";
}

inline void KafkaBus::send_frame(const Frame&) {
    // TODO: implement
    throw TransportNotImplemented("KafkaBus::send_frame");
}

} // namespace aw::tobus
