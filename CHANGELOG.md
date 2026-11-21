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

### Added

- `BinaryEventTraits<T>` — pre-built trait: `std::memcpy` for trivially-copyable types (≤ 255 bytes), or custom `to_binary()` / `from_binary()` for complex types.
- `JsonEventTraits<T>` — pre-built trait serialising to/from JSON via `nlohmann::json` (requires `TOBUS_ENABLE_JSON`).

### Changed

- Integrated reusable workflows and composite actions from `ai-warevo/.github` catalog: CI uses shared `reusable-pr-fast.yml` for fast PR checks and catalog composite actions for Linux builds; Windows MSVC builds use inline CMake commands since shared actions require Ninja which is unavailable on windows-latest.
- Release workflow uses shared `reusable-release.yml` for version resolution, changelog promotion, tag creation, checksum generation/verification, and GitHub Release publication.
- Consumers workflow refactored to test library integration via `add_subdirectory`, `find_package`, and `FetchContent` on Linux clang/gcc and Windows MSVC.
- Pin bundled `aw::tpl` dependency to v3.1.0 so builds use the supported C++20 API and installed package version.
- Run TSan on a native runner with the catalog setup action instead of attempting to change host ASLR from a container.

### Fixed

- Resolved integer sign-conversion warnings in `bus.hpp`, `event_queue.hpp`, and `BinaryEventTraits` serialize/deserialize helpers with explicit casts for `-Wconversion -Wsign-conversion -Wimplicit-int-conversion` compliance.
- CMakeUserPresets.json inheritance path corrected from missing `dev` preset to existing `debug` preset.
- Correct consumer workflow source paths for Linux containers and format all sources exercised by clang-format CI.
- Fix CI-only transport option and example executable paths; make HTTP hex encoding warning-clean under Clang `-Werror`.

## [0.4.0] - 2026-11-20

### Added

- Implemented `HTTPBus` as a bounded in-memory HTTP relay with server/client roles, POST publishing, and client polling.
- Implemented the shared `FramedBus` local delivery, serialization, and receive path.

## [0.3.0] - 2026-11-18

### Added

- add script updater with auto-sync and pre-flight checks

## [0.2.0] - 2026-11-18

### Added

- Initial release

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
