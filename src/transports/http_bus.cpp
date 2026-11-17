#include <aw/tobus/transports/http_bus.hpp>

#include <aw/tobus/errors.hpp>

#include <utility>

namespace aw::tobus {

HTTPBus::HTTPBus(EventBusConfig config)
    : FramedBus(std::move(config)) {
    // TODO: implement (connect / bind using config().endpoint and config().options)
    throw TransportNotImplemented("HTTPBus");
}

HTTPBus::~HTTPBus() = default;

std::string_view HTTPBus::transport_name() const noexcept {
    return "http";
}

void HTTPBus::send_frame(const Frame&) {
    // TODO: implement
    throw TransportNotImplemented("HTTPBus::send_frame");
}

} // namespace aw::tobus
