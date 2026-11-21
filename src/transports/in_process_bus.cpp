#include <aw/tobus/transports/in_process_bus.hpp>

#include "detail/dispatcher.hpp"

#include <utility>

namespace aw::tobus {

struct InProcessBus::Impl {
    explicit Impl(ErrorHandler on_error)
        : dispatcher(detail::Dispatcher::create(std::move(on_error))) {}

    std::shared_ptr<detail::Dispatcher> dispatcher;
};

InProcessBus::InProcessBus(const EventBusConfig& config)
    : impl_(std::make_unique<Impl>(config.on_error)) {}

InProcessBus::~InProcessBus() = default;

Task<void> InProcessBus::drain() {
    return detail::Dispatcher::drain(impl_->dispatcher);
}

std::string_view InProcessBus::transport_name() const noexcept {
    return "in_process";
}

void InProcessBus::publish_raw(const EventTypeInfo& type, std::shared_ptr<const void> event) {
    impl_->dispatcher->dispatch(type, event);
}

Subscription InProcessBus::subscribe_raw(const EventTypeInfo& type, RawEventHandler handler) {
    return impl_->dispatcher->subscribe(type, std::move(handler));
}

} // namespace aw::tobus
