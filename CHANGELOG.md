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

## [Unreleased]

### Added

- `IEventBus`: transport-agnostic bus with typed, non-virtual `publish<T>()` / `subscribe<T>()` over type-erased virtuals; fire-and-forget publish, coroutine handlers (`EventHandler<T>` = `std::function<Task<void>(const T&)>`), `drain()`.
- `EventTypeId` (FNV-1a hash of the declared name), `EventTypeInfo`, `EventTraits<T>`, `NamedEvent` / `SerializableEvent` concepts.
- `InProcessBus`: in-memory transport, no serialization, one shared event object per publish, thread-safe, concurrent handlers.
- `Subscription` (RAII), `EventBusConfig`, `EventBusFactory`, `TransportKind`, and the exceptions `TransportNotAvailable`, `TransportNotImplemented`, `NotSerializable`.
- `FramedBus` foundation and TODO stubs for `UnixSocketBus`, `TCPServerBus`, `HTTPBus`, `WebSocketBus`, `RedisBus`, `ZeroMQBus`, `RabbitMQBus`, `KafkaBus` (CMake options `TOBUS_ENABLE_*`).
- Example `01_in_process` and Catch2 tests.
