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

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace aw::tobus::detail {

// ---- helpers: serialize a single field in big-endian (network) order ----------

inline std::vector<std::byte> serialize_field(std::byte b) noexcept {
    return {b};
}

template <typename Int, std::enable_if_t<std::is_integral_v<Int>, int> = 0>
std::vector<std::byte> serialize_field(Int value) noexcept {
    using U = std::make_unsigned_t<Int>;
    constexpr std::size_t N = sizeof(U);
    std::array<std::byte, N> buf{};
    U v = static_cast<U>(value);
    for (std::size_t i = 0; i < N; ++i)
        buf[N - 1 - i] = std::byte((v >> (8 * i)) & 0xffU);
    return {buf.begin(), buf.end()};
}

inline std::vector<std::byte> serialize_field(float v) noexcept {
    std::array<std::byte, 4> buf{};
    std::memcpy(buf.data(), &v, 4);
    for (int i = 0; i < 2; ++i)
        std::swap(buf[i], buf[3 - i]);
    return {buf.begin(), buf.end()};
}

inline std::vector<std::byte> serialize_field(double v) noexcept {
    std::array<std::byte, 8> buf{};
    std::memcpy(buf.data(), &v, 8);
    for (int i = 0; i < 4; ++i)
        std::swap(buf[i], buf[7 - i]);
    return {buf.begin(), buf.end()};
}

inline std::vector<std::byte> serialize_string(std::string_view sv) {
    std::vector<std::byte> buf;
    buf.reserve(2 + sv.size());
    auto len_bytes = serialize_field(static_cast<std::uint16_t>(static_cast<uint32_t>(sv.size())));
    buf.insert(buf.end(), len_bytes.begin(), len_bytes.end());
    for (char c : sv)
        buf.push_back(std::byte(static_cast<unsigned char>(c)));
    return buf;
}

// ---- helpers: deserialize a single field from big-endian ----------------------

inline std::byte deserialize_field(std::span<const std::byte>& span) {
    if (span.empty()) throw std::runtime_error("binary_event_traits: unexpected end of data");
    auto b = span[0];
    span = span.subspan(1);
    return b;
}

template <typename Int, std::enable_if_t<std::is_integral_v<Int>, int> = 0>
Int deserialize_field(std::span<const std::byte>& span) {
    using U = std::make_unsigned_t<Int>;
    constexpr std::size_t N = sizeof(U);
    if (span.size() < N) throw std::runtime_error("binary_event_traits: insufficient data for integral");
    U v = 0;
    for (std::size_t i = 0; i < N; ++i)
        v = (v << 8) | static_cast<unsigned>(span[i]);
    span = span.subspan(N);
    return static_cast<Int>(v);
}

inline float deserialize_field_float(std::span<const std::byte>& span) {
    if (span.size() < 4) throw std::runtime_error("binary_event_traits: insufficient data for float");
    std::array<std::byte, 4> buf{};
    for (int i = 0; i < 4; ++i)
        buf[3 - i] = span[i];
    span = span.subspan(4);
    float v;
    std::memcpy(&v, buf.data(), 4);
    return v;
}

inline double deserialize_field_double(std::span<const std::byte>& span) {
    if (span.size() < 8) throw std::runtime_error("binary_event_traits: insufficient data for double");
    std::array<std::byte, 8> buf{};
    for (int i = 0; i < 8; ++i)
        buf[7 - i] = span[i];
    span = span.subspan(8);
    double v;
    std::memcpy(&v, buf.data(), 8);
    return v;
}

inline std::string deserialize_string(std::span<const std::byte>& span) {
    auto len = deserialize_field<std::uint16_t>(span);
    if (span.size() < len) throw std::runtime_error("binary_event_traits: insufficient data for string");
    std::string result;
    result.reserve(len);
    for (std::size_t i = 0; i < len; ++i)
        result.push_back(static_cast<char>(span[i]));
    span = span.subspan(len);
    return result;
}

} // namespace aw::tobus::detail

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
