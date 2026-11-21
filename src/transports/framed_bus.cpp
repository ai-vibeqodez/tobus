#include <aw/tobus/transports/framed_bus.hpp>

#include "detail/dispatcher.hpp"

#include <aw/tobus/errors.hpp>

#include <exception>
#include <random>
#include <utility>

namespace aw::tobus {

struct FramedBus::Impl {
    explicit Impl(ErrorHandler on_error)
        : dispatcher(detail::Dispatcher::create(std::move(on_error))) {}

    std::shared_ptr<detail::Dispatcher> dispatcher;
};

namespace {

std::uint64_t random_origin() {
    std::random_device rd;
    std::mt19937_64 gen((static_cast<std::uint64_t>(rd()) << 32) ^ rd());
    return gen();
}

} // namespace

FramedBus::FramedBus(EventBusConfig config)
    : config_(std::move(config)),
      origin_id_(random_origin()),
      impl_(std::make_unique<Impl>(config_.on_error)) {}

FramedBus::~FramedBus() = default;

Task<void> FramedBus::drain() {
    return detail::Dispatcher::drain(impl_->dispatcher);
}

void FramedBus::publish_raw(const EventTypeInfo& type, std::shared_ptr<const void> event) {
    impl_->dispatcher->dispatch(type, event);
    if (!type.serializable()) {
        return;
    }
    try {
        send_frame(Frame{origin_id_, type.id, type.encode(event.get())});
    } catch (...) {
        impl_->dispatcher->report(std::current_exception(), type.name);
    }
}

Subscription FramedBus::subscribe_raw(const EventTypeInfo& type, RawEventHandler handler) {
    require_serializable(type);
    {
        std::lock_guard lock(decoders_mtx_);
        const auto [it, inserted] = event_types_.emplace(type.id, type);
        if (!inserted && it->second.type != type.type) {
            throw std::logic_error("FramedBus: event type id collision for " + std::string(type.name));
        }
    }
    return impl_->dispatcher->subscribe(type, std::move(handler));
}

void FramedBus::receive_frame(const Frame& frame) {
    if (frame.origin == origin_id_) {
        return;
    }
    EventTypeInfo type{};
    {
        std::lock_guard lock(decoders_mtx_);
        const auto it = event_types_.find(frame.type);
        if (it == event_types_.end()) {
            return;
        }
        type = it->second;
    }
    try {
        const auto event = type.decode(frame.payload);
        if (!event) {
            throw std::runtime_error("FramedBus: event decoder returned an empty event");
        }
        impl_->dispatcher->dispatch(type, event);
    } catch (...) {
        impl_->dispatcher->report(std::current_exception(), "unknown");
    }
}

void FramedBus::report_transport_error(const std::exception_ptr& error) noexcept {
    impl_->dispatcher->report(error, transport_name());
}

void FramedBus::require_serializable(const EventTypeInfo& type) {
    if (!type.serializable()) {
        throw NotSerializable(type.name);
    }
}

} // namespace aw::tobus
