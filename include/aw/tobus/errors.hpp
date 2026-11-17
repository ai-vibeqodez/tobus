#pragma once

/**
 * @file errors.hpp
 * @brief Exception types thrown by @c aw::tobus.
 */

#include <aw/tobus/transport_kind.hpp>

#include <stdexcept>
#include <string_view>

namespace aw::tobus {

/// Base class of every exception thrown by tobus itself.
class TobusError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * @brief The requested transport was not compiled in. The message names the CMake option
 *        (e.g. @c TOBUS_ENABLE_REDIS) that enables it.
 */
class TransportNotAvailable final : public TobusError {
public:
    explicit TransportNotAvailable(TransportKind kind);
};

/// The transport exists as a stub only (see the @c TODO markers in its source).
class TransportNotImplemented final : public TobusError {
public:
    /// @param[in] what Transport or operation that is not implemented yet.
    explicit TransportNotImplemented(std::string_view what);
};

/**
 * @brief A network transport was asked to carry an event type that has no @c EventTraits name or codec.
 *        Thrown at @c subscribe() time so the mistake surfaces at startup.
 */
class NotSerializable final : public TobusError {
public:
    /// @param[in] event_name Name of the offending event type.
    explicit NotSerializable(std::string_view event_name);
};

} // namespace aw::tobus
