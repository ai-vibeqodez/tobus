# aw::tobus

Transport-agnostic event bus for C++20 (`aw::tobus`). Modules depend only on `IEventBus`; which transport runs
underneath (in-process today; sockets and brokers later) is a matter of configuration.

```
Module --publish--> IEventBus --subscribe--> Module
                      ^
              "which transport?"  in_process / unix_socket / redis / kafka / ...
```

Depends on [`aw::tpl`](https://github.com/ai-warevo/tpl) (coroutines) and the standard library only.

## Quick start

```cpp
#include <aw/tobus/tobus.hpp>
using namespace aw::tobus;

struct UserJoined { std::string name; };

int main() {
    auto bus = EventBusFactory::create();                 // EventBusConfig{} = in-process

    auto sub = bus->subscribe<UserJoined>([](const UserJoined& e) -> Task<void> {
        std::cout << "welcome " << e.name << "\n";
        co_return;
    });

    bus->publish(UserJoined{"alice"});                    // fire-and-forget: returns immediately
    bus->drain().wait();                                  // optional quiescence point (shutdown, tests)
}
```

## Semantics

| | |
|---|---|
| `publish<T>(event)` | Never waits for handlers. The event is moved into one shared `shared_ptr<const T>`; every handler gets a reference to that object. |
| Handlers | `std::function<Task<void>(const T&)>`, one eager `aw::tpl` task per delivery, so handlers run concurrently. The event stays alive until the handler's task completes, so handlers may suspend. |
| Ordering | None between handlers, nor between two events delivered to one handler. |
| Errors | A throwing handler goes to `EventBusConfig::on_error` (ignored if empty); never to the publisher. |
| `Subscription` | Move-only RAII; destroying it unsubscribes (running handlers are not awaited). It may outlive its bus. |
| `drain()` | `Task<void>` that completes when all deliveries started so far, including cascades, finished. The bus destructor does not block. |

## Serialization boundary

`InProcessBus` never serializes: events are passed by shared pointer. To send an event type over a network
transport, specialise `EventTraits`; tobus defines no wire format, the codec is yours:

```cpp
template <> struct aw::tobus::EventTraits<Ping> {
    static constexpr std::string_view name = "myapp.Ping";       // stable across processes
    static std::vector<std::byte> serialize(const Ping&);
    static Ping deserialize(std::span<const std::byte>);
};
```

Network transports throw `NotSerializable` from `subscribe<T>()` if `T` has no codec (a startup error) and report it
to `on_error` on `publish<T>()` as a safety net. A publisher's own subscribers always receive events directly;
transports drop their own echo using an origin id in the frame header.

## Transports

| Transport | Status | CMake option |
|---|---|---|
| `InProcessBus` | implemented | always on |
| `UnixSocketBus`, `TCPServerBus`, `HTTPBus`, `WebSocketBus` | stub (`// TODO: implement`) | `TOBUS_ENABLE_UNIX_SOCKET`, `_TCP_SERVER`, `_HTTP`, `_WEBSOCKET` |
| `RedisBus`, `ZeroMQBus`, `RabbitMQBus`, `KafkaBus` | header-only stub | `TOBUS_ENABLE_REDIS`, `_ZEROMQ`, `_RABBITMQ`, `_KAFKA` |

`FramedBus` is the planned base of the network transports (send/receive frames; its contract is documented in the
header). Use `InProcessBus` and `detail::Dispatcher` as the reference when implementing one.
A transport that is not compiled in makes `EventBusFactory::create` throw `TransportNotAvailable` naming the option.

## CMake

```cmake
include(FetchContent)
FetchContent_Declare(tobus GIT_REPOSITORY https://github.com/ai-warevo/tobus.git GIT_TAG main)
FetchContent_MakeAvailable(tobus)
target_link_libraries(app PRIVATE aw::tobus)
```

`aw::tpl` is fetched automatically unless the parent project already defines the target.

| Option | Default | |
|---|---|---|
| `TOBUS_BUILD_TESTS` / `TOBUS_BUILD_EXAMPLES` / `TOBUS_INSTALL` | ON top level, OFF as a subproject | |
| `TOBUS_WARNINGS_AS_ERRORS` | OFF | `-Wshadow -Wconversion -Werror` for the library and examples |
| `TOBUS_ENABLE_<TRANSPORT>` | OFF | see above |

## Development

```sh
cp CMakeUserPresets.json.example CMakeUserPresets.json     # local presets (git-ignored); uses /workspace/tpl
cmake --preset dev-local && cmake --build --preset dev-local && ctest --preset dev-local
cmake --preset all-transports && cmake --build --preset all-transports && ctest --preset all-transports   # clang-18 + libc++
```

Toolchain: C++20, CMake 3.23+, clang-18 + libc++ (`-fexperimental-library` is added by `aw::tpl`) or GCC 15 / clang 19 with libstdc++.
Project layout and conventions follow `template-cpp`; see `CHANGELOG.md` for changes.

## License

MIT. See `LICENSE`.
