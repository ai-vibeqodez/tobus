#include <aw/tobus/transports/framed_bus.hpp>

#include <aw/tobus/errors.hpp>

#include <random>
#include <utility>

namespace aw::tobus {

namespace {

std::uint64_t random_origin() {
    std::random_device rd;
    std::mt19937_64 gen((static_cast<std::uint64_t>(rd()) << 32) ^ rd());
    return gen();
}

} // namespace

FramedBus::FramedBus(EventBusConfig config)
    : config_(std::move(config)),
      origin_id_(random_origin()) {}

FramedBus::~FramedBus() = default;

Task<void> FramedBus::drain() {
    // TODO: implement
    throw TransportNotImplemented("FramedBus::drain");
}

void FramedBus::publish_raw(const EventTypeInfo&, std::shared_ptr<const void>) {
    // TODO: implement
    throw TransportNotImplemented("FramedBus::publish");
}

Subscription FramedBus::subscribe_raw(const EventTypeInfo&, RawEventHandler) {
    // TODO: implement
    throw TransportNotImplemented("FramedBus::subscribe");
}

void FramedBus::receive_frame(const Frame&) {
    // TODO: implement
    throw TransportNotImplemented("FramedBus::receive_frame");
}

void FramedBus::require_serializable(const EventTypeInfo& type) {
    if (!type.serializable()) {
        throw NotSerializable(type.name);
    }
}

} // namespace aw::tobus
