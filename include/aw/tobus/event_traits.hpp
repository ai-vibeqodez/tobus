#pragma once

/**
 * @file event_traits.hpp
 * @brief @c EventTraits<T> customisation point, the @c NamedEvent / @c SerializableEvent concepts and
 *        @c event_type_info<T>().
 *
 * Any copyable or movable type can be published on an in-process bus without declaring anything.
 * To send it over a network transport, specialise @c EventTraits:
 * @code
 * template <> struct aw::tobus::EventTraits<Ping> {
 *     static constexpr std::string_view name = "myapp.Ping";
 *     static std::vector<std::byte> serialize(const Ping&);
 *     static Ping deserialize(std::span<const std::byte>);
 * };
 * @endcode
 * tobus defines no wire format: the codec is yours.
 */

#include <aw/tobus/event_type_id.hpp>

#include <concepts>
#include <cstddef>
#include <memory>
#include <span>
#include <string_view>
#include <typeinfo>
#include <vector>

namespace aw::tobus {

/// Primary template: empty. Specialise it to give an event a stable name and a codec.
template <typename T>
struct EventTraits {};

/// @c T has a declared, stable name (@c EventTraits<T>::name).
template <typename T>
concept NamedEvent = requires {
    { EventTraits<T>::name } -> std::convertible_to<std::string_view>;
};

/// @c T is named and has @c serialize / @c deserialize in its traits.
template <typename T>
concept SerializableEvent = NamedEvent<T> && requires(const T& event, std::span<const std::byte> bytes) {
    { EventTraits<T>::serialize(event) } -> std::convertible_to<std::vector<std::byte>>;
    { EventTraits<T>::deserialize(bytes) } -> std::same_as<T>;
};

namespace detail {

template <typename T>
EventTypeInfo make_event_type_info() {
    EventTypeInfo info;
    if constexpr (NamedEvent<T>) {
        info.name = std::string_view{EventTraits<T>::name};
    } else {
        // Process-local identity only: such a type can never be serializable.
        info.name = typeid(T).name();
    }
    info.id = EventTypeId::from_name(info.name);
    info.type = &typeid(T);
    if constexpr (SerializableEvent<T>) {
        info.encode = [](const void* event) -> std::vector<std::byte> {
            return EventTraits<T>::serialize(*static_cast<const T*>(event));
        };
        info.decode = [](std::span<const std::byte> bytes) -> std::shared_ptr<const void> {
            return std::make_shared<const T>(EventTraits<T>::deserialize(bytes));
        };
    }
    return info;
}

} // namespace detail

/**
 * @brief The descriptor of event type @p T. One instance per type for the whole program.
 * @tparam T Unqualified event type.
 */
template <typename T>
[[nodiscard]] const EventTypeInfo& event_type_info() {
    static const EventTypeInfo info = detail::make_event_type_info<T>();
    return info;
}

} // namespace aw::tobus
