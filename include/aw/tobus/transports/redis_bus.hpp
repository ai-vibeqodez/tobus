#ifndef AW_TOBUS_TRANSPORTS_REDIS_BUS_HPP
#define AW_TOBUS_TRANSPORTS_REDIS_BUS_HPP

/**
 * @file redis_bus.hpp
 * @brief @c RedisBus: Redis pub/sub. Needs a client library (e.g. hiredis) when implemented. STUB: declarations only, every body is a TODO.
 */

#include <aw/tobus/errors.hpp>
#include <aw/tobus/transports/framed_bus.hpp>

#include <utility>

namespace aw::tobus {

/**
 * @class RedisBus
 * @brief Not implemented yet. Implement it using @c InProcessBus (delivery) and @c FramedBus (framing) as reference.
 *        Enabled with the CMake option @c TOBUS_ENABLE_REDIS.
 */
class RedisBus final : public FramedBus {
public:
    /// @throws TransportNotImplemented Always (until implemented).
    explicit RedisBus(EventBusConfig config);
    ~RedisBus() override = default;

    [[nodiscard]] std::string_view transport_name() const noexcept override;

protected:
    void send_frame(const Frame& frame) override;
};

inline RedisBus::RedisBus(EventBusConfig config)
    : FramedBus(std::move(config)) {
    // TODO: implement (connect / bind using config().endpoint and config().options)
    throw TransportNotImplemented("RedisBus");
}

inline std::string_view RedisBus::transport_name() const noexcept {
    return "redis";
}

inline void RedisBus::send_frame(const Frame&) {
    // TODO: implement
    throw TransportNotImplemented("RedisBus::send_frame");
}

} // namespace aw::tobus

#endif  // AW_TOBUS_TRANSPORTS_REDIS_BUS_HPP
