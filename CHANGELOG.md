# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

<!--
Maintainer notes (see CLAUDE.md, "Changelog & releases"):
- Add every user-visible change under `## [Unreleased]` in the same commit that makes it.
- Categories: Breaking, Added, Changed, Deprecated, Removed, Fixed, Security. Omit empty ones.
- scripts/release.sh / scripts/release.ps1 derive the next version from those categories.
-->

## [Unreleased]

## [0.1.0] - 2026-11-18

### Added

- `IEventBus`: transport-agnostic bus with typed, non-virtual `publish<T>()` / `subscribe<T>()` over type-erased virtuals; fire-and-forget publish, coroutine handlers (`EventHandler<T>` = `std::function<Task<void>(const T&)>`), `drain()`.
- `EventTypeId` (FNV-1a hash of the declared name), `EventTypeInfo`, `EventTraits<T>`, `NamedEvent` / `SerializableEvent` concepts.
- `InProcessBus`: in-memory transport, no serialization, one shared event object per publish, thread-safe, concurrent handlers.
- `Subscription` (RAII), `EventBusConfig`, `EventBusFactory`, `TransportKind`, and the exceptions `TransportNotAvailable`, `TransportNotImplemented`, `NotSerializable`.
- `FramedBus` foundation and TODO stubs for `UnixSocketBus`, `TCPServerBus`, `HTTPBus`, `WebSocketBus`, `RedisBus`, `ZeroMQBus`, `RabbitMQBus`, `KafkaBus` (CMake options `TOBUS_ENABLE_*`).
- Example `01_in_process` and Catch2 tests.
- CI (`.github/workflows/`): build/test matrix (clang-18 + libc++, g++-15), sanitizers (ASan+UBSan, TSan), 30x repeat of the concurrency tests, clang-format/clang-tidy, coverage, and downstream-consumer checks (`add_subdirectory`, `find_package`, `FetchContent`).
- `tobus_concurrency` CTest entry running the `[threads]` test cases.

### Changed

- A failure with no `EventBusConfig::on_error` is now written to `stderr` instead of being dropped silently; install an empty handler to silence it.
- `publish<T>()` throws `std::logic_error` if another event type with the same `EventTypeId` has subscribers on the bus (previously the payload could be delivered to the wrong type).
- `TransportKind` has an explicit `std::uint8_t` underlying type.
- `detail/dispatcher.hpp` moved from `include/` to `src/detail/`: it is no longer installed or exported.
- `.clang-tidy` is now a narrow check set that fails on any finding.

### Fixed

- `VERSION` was hidden by the template's `version` rule in `.gitignore` (case-insensitive matching), so a `FetchContent` clone could not configure.
- `compile_commands.json` is now exported (clang-tidy needs it).

- `compile_commands.json` is now exported (clang-tidy needs it).
