#ifndef AW_TOBUS_DETAIL_BINARY_EVENT_TRAITS_HELPERS_HPP
#define AW_TOBUS_DETAIL_BINARY_EVENT_TRAITS_HELPERS_HPP

/**
 * @file binary_event_traits_helpers.hpp
 * @brief Internal field encoding helpers used by @c BinaryEventTraits.
 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace aw::tobus::detail {

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
    for (std::size_t i = 0; i < 2; ++i)
        std::swap(buf[i], buf[3 - i]);
    return {buf.begin(), buf.end()};
}

inline std::vector<std::byte> serialize_field(double v) noexcept {
    std::array<std::byte, 8> buf{};
    std::memcpy(buf.data(), &v, 8);
    for (std::size_t i = 0; i < 4; ++i)
        std::swap(buf[i], buf[7 - i]);
    return {buf.begin(), buf.end()};
}

inline std::vector<std::byte> serialize_string(std::string_view sv) {
    std::vector<std::byte> buf;
    buf.reserve(2 + sv.size());
    auto len_bytes = serialize_field(static_cast<std::uint16_t>(static_cast<std::uint32_t>(sv.size())));
    buf.insert(buf.end(), len_bytes.begin(), len_bytes.end());
    for (char c : sv)
        buf.push_back(std::byte(static_cast<unsigned char>(c)));
    return buf;
}

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
        v = static_cast<U>((v << 8) | static_cast<U>(span[i]));
    span = span.subspan(N);
    return static_cast<Int>(v);
}

inline float deserialize_field_float(std::span<const std::byte>& span) {
    if (span.size() < 4) throw std::runtime_error("binary_event_traits: insufficient data for float");
    std::array<std::byte, 4> buf{};
    for (std::size_t i = 0; i < 4; ++i)
        buf[3 - i] = span[i];
    span = span.subspan(4);
    float v;
    std::memcpy(&v, buf.data(), 4);
    return v;
}

inline double deserialize_field_double(std::span<const std::byte>& span) {
    if (span.size() < 8) throw std::runtime_error("binary_event_traits: insufficient data for double");
    std::array<std::byte, 8> buf{};
    for (std::size_t i = 0; i < 8; ++i)
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

#endif  // AW_TOBUS_DETAIL_BINARY_EVENT_TRAITS_HELPERS_HPP
