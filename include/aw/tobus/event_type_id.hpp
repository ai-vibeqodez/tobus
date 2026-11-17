#pragma once

/**
 * @file event_type_id.hpp
 * @brief @c EventTypeId (stable identity of an event type) and @c EventTypeInfo (its type-erased descriptor).
 */

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <typeinfo>
#include <vector>

namespace aw::tobus {

/**
 * @struct EventTypeId
 * @brief 64-bit FNV-1a hash of an event type's declared name. Unlike @c std::type_index it is identical
 *        in every process, so it can travel on the wire.
 */
struct EventTypeId {
    std::uint64_t value = 0;

    /// Hashes @p name (FNV-1a, 64 bit).
    [[nodiscard]] static constexpr EventTypeId from_name(std::string_view name) noexcept {
        std::uint64_t h = 14695981039346656037ULL;
        for (const char c : name) {
            h ^= static_cast<std::uint8_t>(c);
            h *= 1099511628211ULL;
        }
        return EventTypeId{h};
    }

    bool operator==(const EventTypeId&) const noexcept = default;
    auto operator<=>(const EventTypeId&) const noexcept = default;
};

/**
 * @struct EventTypeInfo
 * @brief Type-erased description of one event type, created once per type by @c event_type_info<T>().
 *
 * @c encode / @c decode are set only for @c SerializableEvent types. Transports that stay in memory
 * (@c InProcessBus) never look at them: that is how in-process delivery avoids serialization.
 */
struct EventTypeInfo {
    using EncodeFn = std::vector<std::byte> (*)(const void* event);
    using DecodeFn = std::shared_ptr<const void> (*)(std::span<const std::byte> bytes);

    EventTypeId id;
    std::string_view name;
    const std::type_info* type = nullptr;
    EncodeFn encode = nullptr;
    DecodeFn decode = nullptr;

    /// True if the type can cross a process boundary.
    [[nodiscard]] bool serializable() const noexcept { return encode != nullptr && decode != nullptr; }
};

} // namespace aw::tobus

template <>
struct std::hash<aw::tobus::EventTypeId> {
    std::size_t operator()(const aw::tobus::EventTypeId& id) const noexcept {
        return static_cast<std::size_t>(id.value);
    }
};
