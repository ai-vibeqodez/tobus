#include <aw/tobus/transport_kind.hpp>

namespace aw::tobus {

std::string_view to_string(TransportKind kind) noexcept {
    switch (kind) {
    case TransportKind::in_process:
        return "in_process";
    case TransportKind::unix_socket:
        return "unix_socket";
    case TransportKind::tcp_server:
        return "tcp_server";
    case TransportKind::http:
        return "http";
    case TransportKind::websocket:
        return "websocket";
    case TransportKind::redis:
        return "redis";
    case TransportKind::zeromq:
        return "zeromq";
    case TransportKind::rabbitmq:
        return "rabbitmq";
    case TransportKind::kafka:
        return "kafka";
    }
    return "unknown";
}

std::optional<TransportKind> transport_from_string(std::string_view name) noexcept {
    for (const TransportKind kind : all_transport_kinds) {
        if (to_string(kind) == name) {
            return kind;
        }
    }
    return std::nullopt;
}

} // namespace aw::tobus
