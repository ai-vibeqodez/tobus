#ifndef AW_TOBUS_DETAIL_EVENT_TRAITS_IMPL_HPP
#define AW_TOBUS_DETAIL_EVENT_TRAITS_IMPL_HPP

/**
 * @file event_traits_impl.hpp
 * @brief Internal template implementation used by @c event_type_info().
 *
 * Included by @c event_traits.hpp after the public traits and concepts are declared.
 */

namespace aw::tobus::detail {

template <typename T>
EventTypeInfo make_event_type_info() {
    EventTypeInfo info;
    if constexpr (NamedEvent<T>) {
        info.name = std::string_view{EventTraits<T>::name};
    } else {
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

} // namespace aw::tobus::detail

#endif  // AW_TOBUS_DETAIL_EVENT_TRAITS_IMPL_HPP
