# HTTP relay: three independent processes

This example demonstrates the HTTP transport with a relay server in a separate process and two clients:

| Binary | Role |
|---|---|
| `tobus_example_http_relay_server` | accepts `POST /publish` and serves `GET /poll` |
| `tobus_example_http_relay_publisher` | publishes five `TemperatureReading` events |
| `tobus_example_http_relay_subscriber` | receives and prints events, converting Celsius to Fahrenheit |

## Running

Build the examples with HTTP enabled:

```sh
cmake -S . -B build -DTOBUS_ENABLE_HTTP=ON -DTOBUS_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

In three terminals, start the programs in this order:

```sh
./build/examples/http_relay/tobus_example_http_relay_server
./build/examples/http_relay/tobus_example_http_relay_subscriber
./build/examples/http_relay/tobus_example_http_relay_publisher
```

Pass the endpoint as the first argument to each process. The server expects `host:port`; clients expect `http://host:port`:

```sh
./build/examples/http_relay/tobus_example_http_relay_server 0.0.0.0:9090
./build/examples/http_relay/tobus_example_http_relay_subscriber http://127.0.0.1:9090
./build/examples/http_relay/tobus_example_http_relay_publisher http://127.0.0.1:9090
```

Clients are independent: `publisher` publishes events, while `subscriber` subscribes to them and processes them. The relay stores events in memory; run it only on a trusted network.

## Structure

```
http_relay/
├── CMakeLists.txt
├── include/http_relay/events.hpp
└── src/
    ├── server.cpp
    ├── publisher.cpp
    └── subscriber.cpp
```
