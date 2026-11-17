#include <aw/tobus/transports/in_process_bus.hpp>

#include "detail/dispatcher.hpp"

#include <utility>

namespace aw::tobus {

InProcessBus::InProcessBus(const EventBusConfig& config)
    : dispatcher_(detail::Dispatcher::create(config.on_error)) {}

InProcessBus::~InProcessBus() = default;

Task<void> InProcessBus::drain() {
    return detail::Dispatcher::drain(dispatcher_);
}

std::string_view InProcessBus::transport_name() const noexcept {
    return "in_process";
}

void InProcessBus::publish_raw(const EventTypeInfo& type, std::shared_ptr<const void> event) {
    dispatcher_->dispatch(type, event);
}

Subscription InProcessBus::subscribe_raw(const EventTypeInfo& type, RawEventHandler handler) {
    return dispatcher_->subscribe(type, std::move(handler));
}

} // namespace aw::tobus
