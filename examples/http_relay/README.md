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

## Example output

Here is a typical run — the server listens on port `9090`, the subscriber connects first, and the publisher sends five temperature readings.

### Console output

Three consoles, top to bottom, in chronological order:

```
$ tobus_example_http_relay_server 0.0.0.0:9090

HTTP relay listening on 0.0.0.0:9090; press Ctrl-C to stop
```

```
$ tobus_example_http_relay_subscriber http://127.0.0.1:9090

Listening for temperature readings; press Ctrl-C to stop
received reading #1: 21 C, converted to 69 F
received reading #2: 22 C, converted to 71 F
received reading #3: 23 C, converted to 73 F
received reading #4: 24 C, converted to 75 F
received reading #5: 25 C, converted to 77 F
```

```
$ tobus_example_http_relay_publisher http://127.0.0.1:9090

published reading #1: 21 C
published reading #2: 22 C
published reading #3: 23 C
published reading #4: 24 C
published reading #5: 25 C
```

### Event delivery summary

All five `TemperatureReading` events travelled through the HTTP relay:

| # | Celsius | Fahrenheit |
|---|---------|------------|
| 1 | 21 C    | 69 F       |
| 2 | 22 C    | 71 F       |
| 3 | 23 C    | 73 F       |
| 4 | 24 C    | 75 F       |
| 5 | 25 C    | 77 F       |

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
