#include <aw/tobus/transports/websocket_bus.hpp>

#include <aw/tobus/errors.hpp>

#include <utility>

namespace aw::tobus {

WebSocketBus::WebSocketBus(EventBusConfig config)
    : FramedBus(std::move(config)) {
    // TODO: implement (connect / bind using config().endpoint and config().options)
    throw TransportNotImplemented("WebSocketBus");
}

WebSocketBus::~WebSocketBus() = default;

std::string_view WebSocketBus::transport_name() const noexcept {
    return "websocket";
}

void WebSocketBus::send_frame(const Frame&) {
    // TODO: implement
    throw TransportNotImplemented("WebSocketBus::send_frame");
}

} // namespace aw::tobus
