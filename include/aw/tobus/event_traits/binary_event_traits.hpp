#pragma once

/**
 * @file binary_event_traits.hpp
 * @brief BinaryEventTraits<T> — готовая специализация EventTraits<T>, сериализующая T в сырые байты.
 *
 * Поддерживаются два режима:
 *
 * 1. Trivial mode (по умолчанию). Любой trivially-copyable тип размером <= 256 байт копируется
 *    через std::memcpy. Формат: один байт длины + сырые байты.
 *
 * 2. Field mode. Для нетривиальных типов (например, содержащих std::string) специализируйте
 *    to_binary() и from_binary():
 *
 *    to_binary возвращает std::vector<std::byte>, from_binary принимает span<const std::byte> и T&.
 *
 * Сериализованный payload префиксируется одним байтом длины (макс. 255).
 * BinaryEventTraits — основа для hex-кодированного HTTP-транспорта: транспорт hex-кодирует
 * сырые байты; traits сами производят и потребляют только сырые байты.
 */

#include <aw/tobus/event_traits.hpp>

#include <aw/tobus/detail/binary_event_traits_helpers.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace aw::tobus {

/**
 * @brief Готовая специализация EventTraits<T>, сериализующая T в сырые байты.
 *
 * Для trivially-copyable типов <= 255 байт payload — один байт длины + std::memcpy объекта.
 * Для сложных типов пользователь может специализировать to_binary() / from_binary().
 * @tparam T Event type.
 */
template <typename T>
struct BinaryEventTraits {
    static constexpr std::string_view name = ""; // must be set by the user

    static std::vector<std::byte> serialize(const T& event) {
        constexpr std::size_t MaxSize = 255;
        constexpr bool is_trivial = std::is_trivially_copyable_v<T>;
        constexpr bool has_custom = requires(const T& e) {
            { EventTraits<T>::to_binary(e) } -> std::convertible_to<std::vector<std::byte>>;
        };

        if constexpr (has_custom) {
            auto payload = EventTraits<T>::to_binary(event);
            if (payload.size() > MaxSize) throw std::runtime_error("BinaryEventTraits: payload exceeds 255 bytes");
            std::vector<std::byte> buf;
            buf.reserve(1 + payload.size());
            buf.push_back(std::byte(payload.size()));
            buf.insert(buf.end(), payload.begin(), payload.end());
            return buf;
        } else if constexpr (is_trivial) {
            static_assert(sizeof(T) <= MaxSize, "BinaryEventTraits: trivially-copyable type exceeds 255 bytes");
            std::vector<std::byte> buf(1 + sizeof(T));
            buf[0] = std::byte(sizeof(T));
            std::memcpy(buf.data() + 1, &event, sizeof(T));
            return buf;
        } else {
            static_assert(!is_trivial && !has_custom,
                          "BinaryEventTraits: T is neither trivially-copyable nor has a custom to_binary()");
        }
    }

    static T deserialize(std::span<const std::byte> bytes) {
        if (bytes.empty() || bytes[0] == std::byte(0))
            throw std::runtime_error("BinaryEventTraits: zero-length payload");

        std::size_t payload_size = static_cast<std::size_t>(bytes[0]);
        if (payload_size + 1 > bytes.size()) throw std::runtime_error("BinaryEventTraits: payload size exceeds buffer");

        std::span<const std::byte> payload = bytes.subspan(1, payload_size);

        constexpr bool has_custom = requires(T e, std::span<const std::byte> b) {
            { EventTraits<T>::from_binary(b, e) } -> std::same_as<void>;
        };
        constexpr bool is_trivial = std::is_trivially_copyable_v<T>;

        if constexpr (has_custom) {
            T event{};
            EventTraits<T>::from_binary(payload, event);
            return event;
        } else if constexpr (is_trivial) {
            T event{};
            std::memcpy(&event, payload.data(), payload_size);
            return event;
        } else {
            static_assert(is_trivial || has_custom,
                          "BinaryEventTraits: T is neither trivially-copyable nor has a custom from_binary()");
        }
    }
};

} // namespace aw::tobus
