#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <aw/tobus/event_traits/binary_event_traits.hpp>
#include <aw/tobus/transports/in_process_bus.hpp>
#include <aw/tobus/tobus.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

using namespace aw::tobus;

// ============================================================================
// Trivially-copyable types — roundtrip through BinaryEventTraits
// ============================================================================

namespace {

struct Point2D {
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct TemperatureReading {
    std::uint32_t sequence = 0;
    std::int32_t celsius = 0;
};

struct MixedFields {
    std::uint8_t flags = 0;
    std::int16_t offset = 0;
    std::uint32_t id = 0;
};

} // namespace

template <>
struct aw::tobus::EventTraits<Point2D> : aw::tobus::BinaryEventTraits<Point2D> {
    [[maybe_unused]] static constexpr std::string_view name = "test.Point2D";
};

template <>
struct aw::tobus::EventTraits<TemperatureReading> : aw::tobus::BinaryEventTraits<TemperatureReading> {
    static constexpr std::string_view name = "test.TemperatureReading";
};

template <>
struct aw::tobus::EventTraits<MixedFields> : aw::tobus::BinaryEventTraits<MixedFields> {
    [[maybe_unused]] static constexpr std::string_view name = "test.MixedFields";
};

TEST_CASE("BinaryEventTraits roundtrips a trivially-copyable struct", "[event_traits][binary]") {
    TemperatureReading original{42, -10};
    auto bytes = EventTraits<TemperatureReading>::serialize(original);

    REQUIRE(bytes.size() == 1 + sizeof(TemperatureReading));
    REQUIRE(static_cast<int>(bytes[0]) == sizeof(TemperatureReading));

    auto restored = EventTraits<TemperatureReading>::deserialize(bytes);
    REQUIRE(restored.sequence == 42);
    REQUIRE(restored.celsius == -10);
}

TEST_CASE("BinaryEventTraits roundtrips a multi-field struct", "[event_traits][binary]") {
    MixedFields original{0b10101010, -1234, 0xDEADBEEF};
    auto bytes = EventTraits<MixedFields>::serialize(original);

    auto restored = EventTraits<MixedFields>::deserialize(bytes);
    REQUIRE(restored.flags == 0b10101010);
    REQUIRE(restored.offset == -1234);
    REQUIRE(restored.id == 0xDEADBEEF);
}

TEST_CASE("BinaryEventTraits roundtrips a point struct", "[event_traits][binary]") {
    Point2D p{-500, 3000};
    auto bytes = EventTraits<Point2D>::serialize(p);
    auto restored = EventTraits<Point2D>::deserialize(bytes);
    REQUIRE(restored.x == -500);
    REQUIRE(restored.y == 3000);
}

TEST_CASE("BinaryEventTraits roundtrips zero values", "[event_traits][binary]") {
    Point2D p{};
    auto bytes = EventTraits<Point2D>::serialize(p);
    auto restored = EventTraits<Point2D>::deserialize(bytes);
    REQUIRE(restored.x == 0);
    REQUIRE(restored.y == 0);
}

TEST_CASE("BinaryEventTraits rejects zero-length payload", "[event_traits][binary]") {
    REQUIRE_THROWS_AS(EventTraits<Point2D>::deserialize({}), std::runtime_error);
    std::vector<std::byte> buf(1);
    buf[0] = std::byte(0);
    REQUIRE_THROWS_AS(EventTraits<Point2D>::deserialize(buf), std::runtime_error);
}

TEST_CASE("BinaryEventTraits rejects truncated payload", "[event_traits][binary]") {
    TemperatureReading original{1, 2};
    auto bytes = EventTraits<TemperatureReading>::serialize(original);

    // Truncate last byte
    bytes.pop_back();
    REQUIRE_THROWS_AS(EventTraits<TemperatureReading>::deserialize(bytes), std::runtime_error);
}

// ============================================================================
// Custom to_binary / from_binary for non-trivially-copyable types
// ============================================================================

namespace {

struct MessageEvent {
    std::uint32_t id = 0;
    std::string name;
    double score = 0.0;
};

} // namespace

template <>
struct aw::tobus::EventTraits<MessageEvent> : aw::tobus::BinaryEventTraits<MessageEvent> {
    static constexpr std::string_view name = "test.MessageEvent";

    static std::vector<std::byte> to_binary(const MessageEvent& e) {
        std::vector<std::byte> buf;
        auto id_bytes = detail::serialize_field(e.id);
        buf.insert(buf.end(), id_bytes.begin(), id_bytes.end());
        auto name_bytes = detail::serialize_string(e.name);
        buf.insert(buf.end(), name_bytes.begin(), name_bytes.end());
        auto score_bytes = detail::serialize_field(e.score);
        buf.insert(buf.end(), score_bytes.begin(), score_bytes.end());
        return buf;
    }

    static void from_binary(std::span<const std::byte> span, MessageEvent& e) {
        e.id = detail::deserialize_field<std::uint32_t>(span);
        e.name = detail::deserialize_string(span);
        e.score = detail::deserialize_field_double(span);
    }
};

TEST_CASE("BinaryEventTraits roundtrips a type with custom to_binary/from_binary", "[event_traits][binary]") {
    MessageEvent original{99, "hello", 3.14};
    auto bytes = EventTraits<MessageEvent>::serialize(original);

    REQUIRE(bytes.size() > 1); // at least length byte + payload

    auto restored = EventTraits<MessageEvent>::deserialize(bytes);
    REQUIRE(restored.id == 99);
    REQUIRE(restored.name == "hello");
    REQUIRE(restored.score == Catch::Approx(3.14));
}

TEST_CASE("BinaryEventTraits custom to_binary handles empty string", "[event_traits][binary]") {
    MessageEvent original{1, "", 0.0};
    auto bytes = EventTraits<MessageEvent>::serialize(original);
    auto restored = EventTraits<MessageEvent>::deserialize(bytes);
    REQUIRE(restored.id == 1);
    REQUIRE(restored.name == "");
    REQUIRE(restored.score == Catch::Approx(0.0));
}

TEST_CASE("BinaryEventTraits custom to_binary handles long string", "[event_traits][binary]") {
    std::string long_name(200, 'x');
    MessageEvent original{42, long_name, 1.0};
    auto bytes = EventTraits<MessageEvent>::serialize(original);
    auto restored = EventTraits<MessageEvent>::deserialize(bytes);
    REQUIRE(restored.id == 42);
    REQUIRE(restored.name == long_name);
    REQUIRE(restored.score == Catch::Approx(1.0));
}

TEST_CASE("BinaryEventTraits custom to_binary rejects payload exceeding 255 bytes", "[event_traits][binary]") {
    std::string huge(300, 'x');
    MessageEvent original{1, huge, 0.0};
    REQUIRE_THROWS_AS(EventTraits<MessageEvent>::serialize(original), std::runtime_error);
}

// ============================================================================
// Helper function tests: field serialization / deserialization
// ============================================================================

TEST_CASE("serialize_field serializes integral in big-endian order", "[event_traits][binary]") {
    using namespace aw::tobus::detail;

    auto bytes = serialize_field(static_cast<std::uint32_t>(0x12345678));
    REQUIRE(bytes.size() == 4);
    REQUIRE(static_cast<unsigned>(bytes[0]) == 0x12);
    REQUIRE(static_cast<unsigned>(bytes[1]) == 0x34);
    REQUIRE(static_cast<unsigned>(bytes[2]) == 0x56);
    REQUIRE(static_cast<unsigned>(bytes[3]) == 0x78);
}

TEST_CASE("serialize_field serializes single byte", "[event_traits][binary]") {
    using namespace aw::tobus::detail;

    auto bytes = serialize_field(std::byte(0xAB));
    REQUIRE(bytes.size() == 1);
    REQUIRE(static_cast<unsigned>(bytes[0]) == 0xAB);
}

TEST_CASE("deserialize_field roundtrips integral values", "[event_traits][binary]") {
    using namespace aw::tobus::detail;

    std::vector<std::byte> buf = {std::byte(0x00), std::byte(0x00), std::byte(0x12), std::byte(0x34)};
    std::span<const std::byte> span(buf);
    auto val = deserialize_field<std::uint32_t>(span);
    REQUIRE(val == 0x1234);
    REQUIRE(span.empty());
}

TEST_CASE("deserialize_field roundtrips float", "[event_traits][binary]") {
    using namespace aw::tobus::detail;

    float original = 3.14f;
    auto bytes = serialize_field(original);
    std::span<const std::byte> span_f(bytes);
    auto restored = deserialize_field_float(span_f);
    REQUIRE(restored == Catch::Approx(original));
}

TEST_CASE("deserialize_field roundtrips double", "[event_traits][binary]") {
    using namespace aw::tobus::detail;

    double original = 2.718281828459;
    auto bytes = serialize_field(original);
    std::span<const std::byte> span_d(bytes);
    auto restored = deserialize_field_double(span_d);
    REQUIRE(restored == Catch::Approx(original));
}

TEST_CASE("serialize_string / deserialize_string roundtrip", "[event_traits][binary]") {
    using namespace aw::tobus::detail;

    std::string original = "tobus event bus";
    auto bytes = serialize_string(original);

    std::span<const std::byte> span(bytes);
    auto restored = deserialize_string(span);
    REQUIRE(restored == original);
    REQUIRE(span.empty());
}

TEST_CASE("deserialize_field throws on insufficient data", "[event_traits][binary]") {
    using namespace aw::tobus::detail;

    std::vector<std::byte> buf = {std::byte(0x12), std::byte(0x34)};
    std::span<const std::byte> span(buf);
    REQUIRE_THROWS_AS(deserialize_field<std::uint32_t>(span), std::runtime_error);
}

TEST_CASE("deserialize_string throws on insufficient data", "[event_traits][binary]") {
    using namespace aw::tobus::detail;

    // 2-byte length = 100, but only 1 byte follows
    std::vector<std::byte> buf = {std::byte(0x00), std::byte(0x64), std::byte('x')};
    std::span<const std::byte> span(buf);
    REQUIRE_THROWS_AS(deserialize_string(span), std::runtime_error);
}

// ============================================================================
// Integration with EventBus: publish and receive via in-process bus
// ============================================================================

TEST_CASE("InProcessBus delivers an event serialized with BinaryEventTraits", "[event_traits][binary][transport]") {
    InProcessBus bus;
    std::vector<TemperatureReading> received;

    auto sub = bus.subscribe<TemperatureReading>([&](const TemperatureReading& e) -> Task<void> {
        received.push_back(e);
        co_return;
    });

    bus.publish(TemperatureReading{100, 25});
    bus.drain().wait();

    REQUIRE(received.size() == 1);
    REQUIRE(received[0].sequence == 100);
    REQUIRE(received[0].celsius == 25);
}

TEST_CASE("InProcessBus delivers a custom to_binary event", "[event_traits][binary][transport]") {
    InProcessBus bus;
    std::vector<MessageEvent> received;

    auto sub = bus.subscribe<MessageEvent>([&](const MessageEvent& e) -> Task<void> {
        received.push_back(e);
        co_return;
    });

    bus.publish(MessageEvent{7, "test", 1.5});
    bus.drain().wait();

    REQUIRE(received.size() == 1);
    REQUIRE(received[0].id == 7);
    REQUIRE(received[0].name == "test");
    REQUIRE(received[0].score == Catch::Approx(1.5));
}

// ============================================================================
// JsonEventTraits (only when TOBUS_HAS_JSON is defined)
// ============================================================================

#if TOBUS_HAS_JSON

namespace {

struct JsonEvent {
    std::int32_t value = 0;
    std::string label;

    auto to_json() const {
        nlohmann::json j;
        j["value"] = value;
        j["label"] = label;
        return j;
    }

    static JsonEvent from_json(const nlohmann::json& j) {
        JsonEvent e;
        j.at("value").get_to(e.value);
        j.at("label").get_to(e.label);
        return e;
    }
};

} // namespace

template <>
struct aw::tobus::EventTraits<JsonEvent> : aw::tobus::JsonEventTraits<JsonEvent> {
    static constexpr std::string_view name = "test.JsonEvent";
};

TEST_CASE("JsonEventTraits roundtrips a type with to_json/from_json", "[event_traits][json]") {
    JsonEvent original{42, "hello"};
    auto bytes = EventTraits<JsonEvent>::serialize(original);

    // The serialized bytes should be valid JSON
    std::string json_str;
    json_str.reserve(bytes.size());
    for (auto b : bytes)
        json_str.push_back(static_cast<char>(b));
    REQUIRE(json_str.find("\"value\":42") != std::string::npos || json_str.find("\"value\" : 42") != std::string::npos);

    auto restored = EventTraits<JsonEvent>::deserialize(bytes);
    REQUIRE(restored.value == 42);
    REQUIRE(restored.label == "hello");
}

TEST_CASE("JsonEventTraits roundtrips zero and empty values", "[event_traits][json]") {
    JsonEvent original{0, ""};
    auto bytes = EventTraits<JsonEvent>::serialize(original);
    auto restored = EventTraits<JsonEvent>::deserialize(bytes);
    REQUIRE(restored.value == 0);
    REQUIRE(restored.label == "");
}

TEST_CASE("JsonEventTraits roundtrips nested string content", "[event_traits][json]") {
    JsonEvent original{1, "{\"nested\": \"json\"}"};
    auto bytes = EventTraits<JsonEvent>::serialize(original);
    auto restored = EventTraits<JsonEvent>::deserialize(bytes);
    REQUIRE(restored.value == 1);
    REQUIRE(restored.label == "{\"nested\": \"json\"}");
}

#endif // TOBUS_HAS_JSON
