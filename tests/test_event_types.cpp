#include <catch2/catch_test_macros.hpp>

#include <aw/tobus/tobus.hpp>

#include <string>

using namespace aw::tobus;

namespace {

struct Plain {
    int v = 0;
};

struct Wire {
    int v = 0;
};

struct Named {
    int v = 0;
};

} // namespace

template <>
struct aw::tobus::EventTraits<Wire> {
    static constexpr std::string_view name = "test.Wire";
    static std::vector<std::byte> serialize(const Wire& w) { return {static_cast<std::byte>(w.v)}; }
    static Wire deserialize(std::span<const std::byte> b) { return Wire{static_cast<int>(b.front())}; }
};

template <>
struct aw::tobus::EventTraits<Named> {
    static constexpr std::string_view name = "test.Named";
};

TEST_CASE("EventTypeId is a deterministic hash of the name", "[event_type]") {
    STATIC_REQUIRE(EventTypeId::from_name("a") == EventTypeId::from_name("a"));
    STATIC_REQUIRE(EventTypeId::from_name("a") != EventTypeId::from_name("b"));
    // FNV-1a 64 test vector: hash("") is the offset basis.
    STATIC_REQUIRE(EventTypeId::from_name("").value == 14695981039346656037ULL);
    REQUIRE(std::hash<EventTypeId>{}(EventTypeId{42}) == 42);
    REQUIRE(EventTypeId{1} < EventTypeId{2});
}

TEST_CASE("concepts classify event types", "[event_type]") {
    STATIC_REQUIRE_FALSE(NamedEvent<Plain>);
    STATIC_REQUIRE(NamedEvent<Named>);
    STATIC_REQUIRE_FALSE(SerializableEvent<Named>);
    STATIC_REQUIRE(SerializableEvent<Wire>);
}

TEST_CASE("event_type_info describes a plain type by typeid", "[event_type]") {
    const EventTypeInfo& info = event_type_info<Plain>();
    REQUIRE(&info == &event_type_info<Plain>()); // one instance per type
    REQUIRE(*info.type == typeid(Plain));
    REQUIRE_FALSE(info.serializable());
    REQUIRE(info.id == EventTypeId::from_name(info.name));
}

TEST_CASE("event_type_info uses the declared name and codec", "[event_type]") {
    const EventTypeInfo& named = event_type_info<Named>();
    REQUIRE(named.name == "test.Named");
    REQUIRE_FALSE(named.serializable());

    const EventTypeInfo& wire = event_type_info<Wire>();
    REQUIRE(wire.name == "test.Wire");
    REQUIRE(wire.serializable());
    const Wire w{7};
    const auto bytes = wire.encode(&w);
    const auto back = wire.decode(bytes);
    REQUIRE(static_cast<const Wire*>(back.get())->v == 7);
}
