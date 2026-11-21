#ifndef AW_TOBUS_SUBSCRIPTION_HPP
#define AW_TOBUS_SUBSCRIPTION_HPP

/**
 * @file subscription.hpp
 * @brief @c Subscription: move-only RAII handle that unsubscribes on destruction.
 */

#include <functional>
#include <utility>

namespace aw::tobus {

/**
 * @class Subscription
 * @brief Keeps a handler registered. Destroying (or @c cancel()-ing) it removes the handler.
 *
 * Cancelling stops *new* deliveries; handlers already running are not interrupted or awaited.
 * A subscription may safely outlive its bus (cancelling is then a no-op).
 */
class [[nodiscard]] Subscription {
public:
    using CancelFn = std::function<void()>;

    Subscription() noexcept = default;

    /// Transports build subscriptions from the action that removes the handler.
    explicit Subscription(CancelFn cancel) noexcept
        : cancel_(std::move(cancel)) {}

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;

    Subscription(Subscription&& other) noexcept
        : cancel_(std::exchange(other.cancel_, nullptr)) {}

    Subscription& operator=(Subscription&& other) noexcept {
        if (this != &other) {
            cancel();
            cancel_ = std::exchange(other.cancel_, nullptr);
        }
        return *this;
    }

    ~Subscription() { cancel(); }

    /// Unsubscribes now. Idempotent; never throws.
    void cancel() noexcept {
        if (auto fn = std::exchange(cancel_, nullptr)) {
            try {
                fn();
            } catch (...) {
            }
        }
    }

    /// Detaches the handle: the handler stays registered for the lifetime of the bus.
    void release() noexcept { cancel_ = nullptr; }

    /// True while the handler is registered through this handle.
    [[nodiscard]] bool active() const noexcept { return static_cast<bool>(cancel_); }

private:
    CancelFn cancel_;
};

} // namespace aw::tobus

#endif  // AW_TOBUS_SUBSCRIPTION_HPP
