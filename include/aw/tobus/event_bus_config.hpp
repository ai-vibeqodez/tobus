#pragma once

/**
 * @file event_bus_config.hpp
 * @brief @c EventBusConfig: everything that selects and tunes a transport. Module code never sees it.
 */

#include <aw/tobus/event_handler.hpp>
#include <aw/tobus/transport_kind.hpp>

#include <map>
#include <string>

namespace aw::tobus {

/**
 * @struct EventBusConfig
 * @brief Flat on purpose: it maps 1:1 onto a config file and does not depend on any transport header.
 */
struct EventBusConfig {
    /// Which transport to create.
    TransportKind transport = TransportKind::in_process;

    /// Transport address: socket path, @c host:port, URL, broker list, ... Ignored by @c in_process.
    std::string endpoint;

    /// Transport-specific settings (documented by each transport).
    std::map<std::string, std::string> options;

    /**
     * Receives failures nobody else can observe (a throwing handler, a failed send).
     *
     * Empty (the default) means: write one line per failure to @c std::cerr, naming the event type. Errors are never
     * dropped silently by accident; to silence them on purpose, install a handler that does nothing
     * (@c cfg.on_error = [](std::exception_ptr) {};).
     *
     * Called from pool threads, possibly concurrently: it must be thread-safe, and anything it throws is discarded.
     */
    ErrorHandler on_error;
};

} // namespace aw::tobus
