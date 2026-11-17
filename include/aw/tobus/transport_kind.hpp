#pragma once

/**
 * @file transport_kind.hpp
 * @brief @c TransportKind: the transports @c EventBusFactory can create, and their string names.
 */

#include <array>
#include <optional>
#include <cstdint>
#include <string_view>

namespace aw::tobus {

/**
 * @enum TransportKind
 * @brief Selects the transport behind an @c IEventBus. Module code never names a transport; only configuration does.
 */
enum class TransportKind : std::uint8_t {
    in_process,
    unix_socket,
    tcp_server,
    http,
    websocket,
    redis,
    zeromq,
    rabbitmq,
    kafka,
};

/// Every transport kind, in declaration order.
inline constexpr std::array<TransportKind, 9> all_transport_kinds{
    TransportKind::in_process, TransportKind::unix_socket, TransportKind::tcp_server,
    TransportKind::http,       TransportKind::websocket,   TransportKind::redis,
    TransportKind::zeromq,     TransportKind::rabbitmq,    TransportKind::kafka,
};

/**
 * @brief Canonical lower-case name of @p kind (e.g. @c "unix_socket"); also the config-file spelling.
 */
[[nodiscard]] std::string_view to_string(TransportKind kind) noexcept;

/**
 * @brief Parses a name produced by @c to_string().
 * @return The kind, or @c std::nullopt if @p name is unknown.
 */
[[nodiscard]] std::optional<TransportKind> transport_from_string(std::string_view name) noexcept;

} // namespace aw::tobus
