#pragma once

/**
 * @file tobus.hpp
 * @brief Umbrella header: everything a module needs to publish and subscribe, and everything an application
 *        needs to create a bus. Transport classes are deliberately NOT included: pick them through
 *        @c EventBusFactory (or include @c aw/tobus/transports/... explicitly).
 */

#include <aw/tobus/errors.hpp>
#include <aw/tobus/event_bus.hpp>
#include <aw/tobus/event_bus_config.hpp>
#include <aw/tobus/event_bus_factory.hpp>
#include <aw/tobus/event_handler.hpp>
#include <aw/tobus/event_traits.hpp>
#include <aw/tobus/event_type_id.hpp>
#include <aw/tobus/subscription.hpp>
#include <aw/tobus/transport_kind.hpp>
