#include <aw/tobus/detail/dispatcher.hpp>

#include <aw/tpl/threading/tasks/spawn.hpp>

#include <stdexcept>
#include <string>
#include <utility>

namespace aw::tobus::detail {

Dispatcher::Dispatcher(PrivateTag, ErrorHandler on_error)
    : on_error_(std::move(on_error)) {}

std::shared_ptr<Dispatcher> Dispatcher::create(ErrorHandler on_error) {
    return std::make_shared<Dispatcher>(PrivateTag{}, std::move(on_error));
}

Subscription Dispatcher::subscribe(const EventTypeInfo& type, RawEventHandler handler) {
    auto shared_handler = std::make_shared<RawEventHandler>(std::move(handler));
    std::uint64_t token = 0;
    {
        std::lock_guard lock(mtx_);
        const auto [it, inserted] = identities_.try_emplace(type.id, type.type);
        // The id is a hash of the name: two different C++ types sharing it would receive each other's events.
        if (!inserted && *it->second != *type.type) {
            throw std::logic_error("tobus: event type id collision for '" + std::string(type.name) +
                                   "': give each event type a unique EventTraits<T>::name");
        }
        token = next_token_++;
        auto next = std::make_shared<Table>(*table_);
        (*next)[type.id].push_back(Entry{token, std::move(shared_handler)});
        table_ = std::move(next);
    }
    return Subscription([weak = weak_from_this(), id = type.id, token] {
        if (auto self = weak.lock()) {
            self->unsubscribe(id, token);
        }
    });
}

void Dispatcher::unsubscribe(EventTypeId id, std::uint64_t token) noexcept {
    try {
        std::lock_guard lock(mtx_);
        auto next = std::make_shared<Table>(*table_);
        const auto it = next->find(id);
        if (it == next->end()) {
            return;
        }
        std::erase_if(it->second, [token](const Entry& e) { return e.token == token; });
        if (it->second.empty()) {
            next->erase(it);
        }
        table_ = std::move(next);
    } catch (...) { // NOLINT(bugprone-empty-catch): only bad_alloc; the handler then simply stays registered
    }
}

std::size_t Dispatcher::subscriber_count(EventTypeId id) const {
    std::shared_ptr<const Table> snapshot;
    {
        std::lock_guard lock(mtx_);
        snapshot = table_;
    }
    const auto it = snapshot->find(id);
    return it == snapshot->end() ? 0 : it->second.size();
}

void Dispatcher::dispatch(const EventTypeInfo& type, std::shared_ptr<const void> event) {
    std::shared_ptr<const Table> snapshot;
    {
        std::lock_guard lock(mtx_);
        snapshot = table_;
    }
    const auto it = snapshot->find(type.id);
    if (it == snapshot->end()) {
        return;
    }
    auto self = shared_from_this();
    for (const Entry& entry : it->second) {
        // Count before starting: run() may finish (and call done()) on another thread immediately.
        inflight_.add();
        try {
            aw::tpl::threading::tasks::co_spawn(run(self, entry.handler, event));
        } catch (...) {
            inflight_.done();
            report(std::current_exception());
        }
    }
}

// The frame owns `self`, the handler and the event until the end: the handler's `const T&` can never dangle,
// and the group (a member of *self) outlives the final done().
Task<void> Dispatcher::run(std::shared_ptr<Dispatcher> self, std::shared_ptr<RawEventHandler> handler,
                           std::shared_ptr<const void> event) {
    try {
        co_await (*handler)(event.get());
    } catch (...) {
        self->report(std::current_exception());
    }
    handler.reset();
    event.reset();
    self->inflight_.done();
}

Task<void> Dispatcher::drain(std::shared_ptr<Dispatcher> self) {
    co_await self->inflight_.wait_async();
}

void Dispatcher::report(std::exception_ptr ep) noexcept {
    if (!on_error_) {
        return;
    }
    try {
        on_error_(ep);
    } catch (...) { // NOLINT(bugprone-empty-catch): a throwing error handler must not take the bus down
    }
}

} // namespace aw::tobus::detail
