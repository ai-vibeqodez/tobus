#pragma once

/**
 * @file in_process_bus.hpp
 * @brief @c InProcessBus: the reference transport. Events stay in memory; nothing is serialized.
 */

#include <aw/tobus/event_bus.hpp>
#include <aw/tobus/event_bus_config.hpp>

#include <memory>

namespace aw::tobus {

/**
 * @class InProcessBus
 * @brief Delivers events to subscribers of the same process.
 *
 * The published object is moved into one @c shared_ptr<const T> and every handler receives a reference to that
 * very object: no copy per subscriber, and @c EventTypeInfo::encode / @c decode are never touched, so event types
 * need no serialization support. Thread-safe. Later transports are modelled on this class.
 */
class InProcessBus final : public IEventBus {
public:
    /// Only @c config.on_error is used.
    explicit InProcessBus(const EventBusConfig& config = {});
    ~InProcessBus() override;

    [[nodiscard]] Task<void> drain() override;
    [[nodiscard]] std::string_view transport_name() const noexcept override;

protected:
    void publish_raw(const EventTypeInfo& type, std::shared_ptr<const void> event) override;
    Subscription subscribe_raw(const EventTypeInfo& type, RawEventHandler handler) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace aw::tobus
