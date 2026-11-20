#pragma once

#include <aw/tobus/tobus.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace http_relay {

struct TemperatureReading {
    std::uint32_t sequence = 0;
    std::int32_t celsius = 0;
};

} // namespace http_relay

template <>
struct aw::tobus::EventTraits<http_relay::TemperatureReading> {
    static constexpr std::string_view name = "example.http_relay.TemperatureReading";

    static std::vector<std::byte> serialize(const http_relay::TemperatureReading& event) {
        const auto sequence = event.sequence;
        const auto celsius = static_cast<std::uint32_t>(event.celsius);
        return {static_cast<std::byte>((sequence >> 24) & 0xffU), static_cast<std::byte>((sequence >> 16) & 0xffU),
                static_cast<std::byte>((sequence >> 8) & 0xffU), static_cast<std::byte>(sequence & 0xffU),
                static_cast<std::byte>((celsius >> 24) & 0xffU), static_cast<std::byte>((celsius >> 16) & 0xffU),
                static_cast<std::byte>((celsius >> 8) & 0xffU), static_cast<std::byte>(celsius & 0xffU)};
    }

    static http_relay::TemperatureReading deserialize(std::span<const std::byte> bytes) {
        if (bytes.size() != 8) throw std::runtime_error("invalid TemperatureReading payload");
        const auto read_uint32 = [&bytes](std::size_t offset) {
            return (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset])) << 24) |
                   (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 1])) << 16) |
                   (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 2])) << 8) |
                   static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 3]));
        };
        return {read_uint32(0), static_cast<std::int32_t>(read_uint32(4))};
    }
};
