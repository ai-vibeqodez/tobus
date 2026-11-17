#include <aw/tobus/transports/tcp_server_bus.hpp>

#include <aw/tobus/errors.hpp>

#include <utility>

namespace aw::tobus {

TCPServerBus::TCPServerBus(EventBusConfig config)
    : FramedBus(std::move(config)) {
    // TODO: implement (connect / bind using config().endpoint and config().options)
    throw TransportNotImplemented("TCPServerBus");
}

TCPServerBus::~TCPServerBus() = default;

std::string_view TCPServerBus::transport_name() const noexcept {
    return "tcp_server";
}

void TCPServerBus::send_frame(const Frame&) {
    // TODO: implement
    throw TransportNotImplemented("TCPServerBus::send_frame");
}

} // namespace aw::tobus
