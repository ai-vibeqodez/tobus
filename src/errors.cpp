#include <aw/tobus/errors.hpp>

#include <string>

namespace aw::tobus {

namespace {

std::string option_name(TransportKind kind) {
    std::string name = "TOBUS_ENABLE_";
    for (const char c : to_string(kind)) {
        name += static_cast<char>(c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c);
    }
    return name;
}

} // namespace

TransportNotAvailable::TransportNotAvailable(TransportKind kind)
    : TobusError("tobus: transport '" + std::string(to_string(kind)) +
                 "' was not compiled in; enable the CMake option " + option_name(kind)) {}

TransportNotImplemented::TransportNotImplemented(std::string_view what)
    : TobusError("tobus: " + std::string(what) + " is not implemented yet") {}

NotSerializable::NotSerializable(std::string_view event_name)
    : TobusError("tobus: event type '" + std::string(event_name) +
                 "' cannot be sent over this transport: specialise aw::tobus::EventTraits<T> with name, serialize and "
                 "deserialize") {}

} // namespace aw::tobus
