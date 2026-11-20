# 🧩 Example: chat built from independent modules

Full source: [`src/main.cpp`](src/main.cpp), built as `tobus_example_chat`.

This example uses the **in_process** transport (events are passed on a single thread through a thread pool). Separate examples for `unix_socket`, `redis`, `kafka`, and other transports will be added in future directories such as `02_websocket`, `03_unix_socket`, and so on.

## Scenario

Two users join the chat and send messages. Each part of the system is a separate module class that receives only `IEventBus&` and knows nothing about its neighbors:

| Module | Publishes | Subscribes to | What it does |
|---|---|---|---|
| `Auth` | `UserJoined` | — | logs a user in |
| `Room` | `MessageSent` | `UserJoined`, `MessageApproved` | accepts messages from users and displays only approved ones |
| `Moderation` | `MessageApproved` or `MessageBlocked` | `MessageSent` | checks the text (with a delay, as if using an external filter) and issues a verdict |
| `Notifier` | — | `MessageBlocked` | tells the author that their message was rejected |
| `Stats` | — | all four events | counts events without publishing anything |

## Event cascade

What happens when `room.say("bob", "buy cheap spam now")`:

```
Room.say()  ──MessageSent──►  Moderation  (waits 20 ms, sees «spam»)
                                  │
                                  └──MessageBlocked──►  Notifier  → «@bob: your message was blocked»
                                                    └─►  Stats
            └────────────────►  Stats                  (Room never displays this message)
```

## Step by step

1. `Room` publishes a raw `MessageSent` event and immediately returns. It does not wait for moderation or know that moderation exists.
2. `Moderation` receives the event and **publishes a new one in response** (`MessageApproved` or `MessageBlocked`). This is an event cascade: a handler can also be a publisher.
3. For a normal message, `Room` receives `MessageApproved` and prints it. For spam, `Notifier` warns the author, and the message does not appear in the room.
4. `Stats` listens to every event and was added without changing any other module. The same approach works for logging, auditing, and metrics.
5. `main` is the only place that knows about the transport (`config.transport`). Changing it moves the same modules to a different transport.

## Sample output

```
transport: in_process
[room]      * alice joined
[room]      * bob joined
[room]      alice: hello everyone!
[room]      alice: welcome, bob
[notifier]  @bob: your message was blocked (looks like spam)

joined=2 sent=3 approved=2 blocked=1
```

The order of output lines varies between runs; this is expected.

## Structure

```
chat/
├── CMakeLists.txt
├── include/chat/          — headers (event types + module interfaces)
│   ├── events.hpp
│   ├── auth.hpp
│   ├── room.hpp
│   ├── moderation.hpp
│   ├── notifier.hpp
│   └── stats.hpp
└── src/                   — implementations
    ├── main.cpp           — composition root, transport
    ├── room.cpp
    ├── moderation.cpp
    ├── notifier.cpp
    └── stats.cpp
```

## What to note

- **Everything is asynchronous.** `publish` does not wait; handlers run in parallel on a thread pool. Output order is not guaranteed, output is protected by a mutex, and `Stats` counters are `atomic`.
- **`co_await` suspends only its own handler.** While `Moderation` "thinks" for 20 ms, other events and handlers continue without waiting.
- **`drain()` waits for cascades too.** It completes only after both started deliveries and deliveries created by handlers (moderation verdicts) have finished.
- **A subscription lasts as long as its `Subscription` object.** Module `Subscription` fields automatically unsubscribe handlers when the module is destroyed.
- **Errors do not reach the publisher.** Exceptions from any handler are passed to `config.on_error`.
