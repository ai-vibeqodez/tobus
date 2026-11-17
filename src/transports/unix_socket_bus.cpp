#include <aw/tobus/transports/unix_socket_bus.hpp>

#include <aw/tobus/errors.hpp>

#include <utility>

namespace aw::tobus {

UnixSocketBus::UnixSocketBus(EventBusConfig config)
    : FramedBus(std::move(config)) {
    // TODO: implement (connect / bind using config().endpoint and config().options)
    throw TransportNotImplemented("UnixSocketBus");
}

UnixSocketBus::~UnixSocketBus() = default;

std::string_view UnixSocketBus::transport_name() const noexcept {
    return "unix_socket";
}

void UnixSocketBus::send_frame(const Frame&) {
    // TODO: implement
    throw TransportNotImplemented("UnixSocketBus::send_frame");
}

} // namespace aw::tobus
