# Examples

Each example is a separate directory with its own `CMakeLists.txt`.

| Example | Description |
|---|---|
| [chat](chat/README.md) | Event-driven chat: several independent modules connected through an event bus (the **in_process** transport) |
| [http_relay](http_relay/README.md) | HTTP relay with separate server, publisher, and subscriber processes (requires `TOBUS_ENABLE_HTTP=ON`) |
