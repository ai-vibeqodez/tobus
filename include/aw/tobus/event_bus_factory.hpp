#pragma once

/**
 * @file event_bus_factory.hpp
 * @brief @c EventBusFactory: turns an @c EventBusConfig into an @c IEventBus.
 */

#include <aw/tobus/event_bus.hpp>
#include <aw/tobus/event_bus_config.hpp>

#include <memory>

namespace aw::tobus {

/**
 * @class EventBusFactory
 * @brief Plain dispatch over @c TransportKind. Transports are compiled in with the CMake options
 *        @c TOBUS_ENABLE_<TRANSPORT>; there is no runtime self-registration (it would be dropped by static linking).
 */
class EventBusFactory {
public:
    /**
     * @brief Creates the bus selected by @c config.transport.
     * @throws TransportNotAvailable If that transport was not compiled in.
     * @throws TransportNotImplemented If it is compiled in but still a stub.
     */
    [[nodiscard]] static std::shared_ptr<IEventBus> create(const EventBusConfig& config = {});

    /// True if @p kind was compiled in (it may still be a stub).
    [[nodiscard]] static bool is_available(TransportKind kind) noexcept;
};

} // namespace aw::tobus
