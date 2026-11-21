#ifndef AW_TOBUS_EVENT_TRAITS_HPP
#define AW_TOBUS_EVENT_TRAITS_HPP

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
 *
 * Pre-built specialisations are available in @c aw/tobus/event_traits/:
 * - @c BinaryEventTraits<T> — serialises trivially-copyable types (≤ 255 bytes) with @c std::memcpy,
 *   or supports custom @c to_binary()/@c from_binary() for complex types.
 * - @c JsonEventTraits<T> — serialises to/from JSON via @c nlohmann/json. Requires @c TOBUS_ENABLE_JSON.
 *
 * Example with a pre-built trait:
 * @code
 * struct TemperatureReading { std::uint32_t sequence; std::int32_t celsius; };
 *
 * template <> struct aw::tobus::EventTraits<TemperatureReading>
 *     : aw::tobus::BinaryEventTraits<TemperatureReading> {
 *     static constexpr std::string_view name = "example.http_relay.TemperatureReading";
 * };
 * @endcode
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

} // namespace aw::tobus

#include <aw/tobus/detail/event_traits_impl.hpp>

namespace aw::tobus {

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

#endif  // AW_TOBUS_EVENT_TRAITS_HPP
