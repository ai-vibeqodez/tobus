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
 *
 * What is guaranteed:
 *  - Within one process two different C++ types never share an id silently: the bus compares @c std::type_info on
 *    @c subscribe() and @c publish() and throws @c std::logic_error on a clash. A wrong-type delivery cannot happen.
 *  - The hash is not collision-free: with 64 bits the chance that two of N names collide is about N^2 / 2^65
 *    (about 3e-14 for 1000 event types). Colliding names are fixed by renaming one of them.
 *
 * What is not: across processes only the id travels, so a collision between types of two different programs
 * cannot be detected locally. Decoding a payload as the wrong type is then the codec's problem; transports that
 * need more safety can add the name (or a schema hash) to the frame header without changing this type.
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
