# 🚌 aw::tobus

> `auto bus = aw::tobus::EventBusFactory::create();` — A fast event shuttle: inside the process it runs direct without transfers or serialization, and when needed it easily takes intercity network routes.

A transport-agnostic event bus for **C++20** (`aw::tobus`). Modules depend only on the `IEventBus` interface; the transport choice under the hood (in-process "shuttle" today; sockets and brokers tomorrow) is determined solely by configuration.

Use this code with caution.

```text
Module --publish--> IEventBus --subscribe--> Module
                      ^
              "which route?"  in_process / unix_socket / redis / kafka / ...
```

Depends only on coroutines from [`aw::tpl`](https://github.com/ai-warevo/tpl) and the C++ standard library.

## 💡 Why do you need this

Normally, module A calls module B directly: A knows about B, includes its header, and waits for a response. When there are many modules, everything gets tangled.

With a bus, modules don't know about each other. One **publishes an event** ("user logged in", "message received"), others are **subscribed** to it and react. The publisher doesn't know how many listeners there are for an event, or whether there are any at all. A new listener can be added without modifying existing code.

```
 Auth ──UserJoined──┐
                    ├─► IEventBus ──► Room, Stats, ...   (whoever is subscribed receives it)
 Room ──MessageSent─┘
```

## 🚀 Quick Start

```cpp
#include <aw/tobus/tobus.hpp>
using namespace aw::tobus;

struct UserJoined { std::string name; };                  // event — just a struct

int main() {
    auto bus = EventBusFactory::create();                 // EventBusConfig{} = in-process

    // Subscribe: "when someone publishes UserJoined — call this function".
    // The handler is a coroutine (Task<void>), so you can co_await inside.
    auto sub = bus->subscribe<UserJoined>([](const UserJoined& e) -> Task<void> {
        std::cout << "hello, " << e.name << "\n";
        co_return;
    });                                                   // sub lives — subscription is active; destroy it — unsubscribe

    bus->publish(UserJoined{"alice"});                    // fire-and-forget: returns immediately, handler runs in the background
    bus->drain().wait();                                  // wait for all started handlers (on shutdown and in tests)
}
```

## 🧩 Example

A full walkthrough is in [`examples/README.md`](examples/README.md).

## ⚙️ Semantics

| Mechanism | Behavior |
|---|---|
| `publish<T>(event)` | Never waits for handlers. The event is moved into a single `shared_ptr<const T>`; each passenger-subscriber gets a const reference to that object. |
| Handlers | `std::function<Task<void>(const T&)>` — one eager `aw::tpl` task per delivery, so handlers run concurrently. The event lives until the task completes, so the handler can safely go into `co_await`. |
| Delivery order | Not guaranteed — neither between different handlers, nor between two events for the same handler. |
| Errors | Exceptions from handlers are forwarded to `EventBusConfig::on_error` and never propagate to the publisher. If the callback is unset, the error is written as a single line to `stderr` (with the event name) rather than silently lost; to consciously suppress it, set an empty handler: `cfg.on_error = [](const std::exception_ptr&) {};`. The callback is invoked from pool threads, possibly in parallel. |
| `Subscription` | Move-only RAII ticket; on destruction it unsubscribes the listener (already-started handlers are not cancelled or awaited). May outlive the bus object itself. |
| `drain()` | Returns a `Task<void>` that completes when all started deliveries have finished, including cascaded events. The bus destructor does not block the thread. There is no `shutdown()` method: `drain()` is the silence point, and cancellation of already-started handlers is not supported. Network transports close connections in the destructor; an async `close()` will appear alongside them as a non-pure-virtual method (defaulting to `drain()`), without breaking the existing API. |
| Type identity | `EventTypeId` is a 64-bit FNV-1a hash of `EventTraits<T>::name`. Two different C++ types with the same id within a process are detected at `subscribe` and `publish` (via `std::type_info`) and result in `std::logic_error`, so an event will never be delivered to the "wrong" type. Across processes over the network only the id is transmitted: collisions between different programs are not locally detectable (probability ≈ N²/2⁶⁵; if needed, a transport can add a name or schema hash to the frame header). |

## 📦 Serialization Boundary

`InProcessBus` never serializes data: events travel inside the process via `shared_ptr` without extra copying. To route an event over the network, specialize the `EventTraits` template — `tobus` does not impose a data format; the codec is up to you:

```cpp
template <> struct aw::tobus::EventTraits<Ping> {
    static constexpr std::string_view name = "myapp.Ping";       // stable route name across processes
    static std::vector<std::byte> serialize(const Ping&);
    static Ping deserialize(std::span<const std::byte>);
};
```

Network transports throw `NotSerializable` on `subscribe<T>()` if no codec is defined for `T` (a startup configuration error), and forward it to `on_error` on `publish<T>()` as a safeguard. Local subscribers of the publisher always receive events directly; transports discard their own echo based on the `origin id` in the frame header.

## 🚏 Transports (Routes)

| Transport | Status | CMake Option |
|---|---|---|
| `InProcessBus` | implemented | always enabled |
| `UnixSocketBus`, `TCPServerBus`, `HTTPBus`, `WebSocketBus` | stub (`// TODO: implement`) | `TOBUS_ENABLE_UNIX_SOCKET`, `_TCP_SERVER`, `_HTTP`, `_WEBSOCKET` |
| `RedisBus`, `ZeroMQBus`, `RabbitMQBus`, `KafkaBus` | header-only stub | `TOBUS_ENABLE_REDIS`, `_ZEROMQ`, `_RABBITMQ`, `_KAFKA` |

`FramedBus` is designed as a base class for network transports (sending and receiving frames; the contract is described in the header). When implementing new transports, use `InProcessBus` and `detail::Dispatcher` as a reference.

Attempting to create an uncompiled transport via `EventBusFactory::create` will throw `TransportNotAvailable` with the required CMake option indicated.

## 🛠 CMake Integration

```cmake
include(FetchContent)
FetchContent_Declare(tobus GIT_REPOSITORY https://github.com/ai-warevo/tobus.git GIT_TAG main)
FetchContent_MakeAvailable(tobus)
target_link_libraries(app PRIVATE aw::tobus)
```

The `aw::tpl` library is pulled in automatically if the target is not already defined in the parent project.

| Option | Default | Description |
|---|---|---|
| `TOBUS_BUILD_TESTS` / `TOBUS_BUILD_EXAMPLES` / `TOBUS_INSTALL` | `ON` for root project, `OFF` as subproject | Build tests, examples, and install rules |
| `TOBUS_WARNINGS_AS_ERRORS` | `OFF` | Enables `-Wshadow -Wconversion -Werror` for the library and examples |
| `TOBUS_ENABLE_<TRANSPORT>` | `OFF` | Enable specific transports (see table above) |

## 💻 Development & Build

```sh
cp CMakeUserPresets.json.example CMakeUserPresets.json     # local presets (in .gitignore)
cmake --preset dev && cmake --build --preset dev && ctest --preset dev
cmake --preset clang18-libcxx && cmake --build --preset clang18-libcxx && ctest --preset clang18-libcxx
```

## 📄 License

MIT. See the `LICENSE` file for details.
