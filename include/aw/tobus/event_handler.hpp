#pragma once

/**
 * @file event_handler.hpp
 * @brief Handler aliases. Handlers are coroutines from @c aw::tpl.
 */

#include <aw/tpl/threading/tasks/spawn.hpp>
#include <aw/tpl/threading/tasks/task.hpp>

#include <functional>

namespace aw::tobus {

/// The eager, ref-counted coroutine task of @c aw::tpl.
using aw::tpl::threading::tasks::Task;

/**
 * @brief Receives an event. The reference is valid until the returned task completes: the bus keeps the
 *        event alive for as long as the handler runs, so handlers may suspend freely.
 */
template <typename T>
using EventHandler = std::function<Task<void>(const T&)>;

/// Type-erased handler used between the typed facade and transports (@c const @c void* points at the event).
using RawEventHandler = std::function<Task<void>(const void*)>;

/// Receives exceptions that cannot be returned to anyone (a failing handler, a failing send).
using ErrorHandler = aw::tpl::threading::tasks::ErrorHandler;

} // namespace aw::tobus
