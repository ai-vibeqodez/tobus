#pragma once

#include <aw/tobus/tobus.hpp>
#include <aw/tobus/event_traits/binary_event_traits.hpp>

#include <cstdint>

namespace http_relay {

struct TemperatureReading {
    std::uint32_t sequence = 0;
    std::int32_t celsius = 0;
};

} // namespace http_relay

template <>
struct aw::tobus::EventTraits<http_relay::TemperatureReading>
    : aw::tobus::BinaryEventTraits<http_relay::TemperatureReading> {
    static constexpr std::string_view name = "example.http_relay.TemperatureReading";
};
