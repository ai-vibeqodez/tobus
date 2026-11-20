# Testing http_relay example via curl

## Overview

This document describes how to test the HTTP relay example using `curl` instead of the official publisher/subscriber binaries. The HTTP transport exposes two endpoints:

| Method | Path | Description |
|--------|------|-------------|
| `POST` | `/publish` | Accepts a hex-encoded frame |
| `GET` | `/poll?after={cursor}` | Returns frames after the given cursor |

## Frame format

Frames are hex-encoded binary data with the following structure:

```
+----------------+----------------+----------------+----------------+
| origin (8 B)   | type_id (8 B)  | payload_size   | payload (N B)  |
| uint64 BE      | uint64 BE      | uint32 BE      | variable       |
+----------------+----------------+----------------+----------------+
```

For `TemperatureReading` events:
- **origin**: always `0000000000000000`
- **type_id**: `1a484de120e8c575` (SHA-256 hash of the event name, truncated)
- **payload_size**: `00000008` (8 bytes = 4 + 4)
- **payload**: `sequence` (4 B BE) + `celsius` (4 B BE)

Total frame size: **28 bytes = 56 hex characters**

## Building

```sh
cmake -S . -B build -DTOBUS_ENABLE_HTTP=ON -DTOBUS_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

## Running the server

```sh
./build/examples/http_relay/tobus_example_http_relay_server 0.0.0.0:9092
```

## Publishing events

### Single event (curl)

```sh
# seq=1, celsius=21
curl -X POST http://127.0.0.1:9092/publish \
  -d "00000000000000001a484de120e8c575000000080000000100000015"
# Response: "ok" (HTTP 200)
```

### Five events (bash loop, matching the official publisher)

```sh
for i in 1 2 3 4 5; do
    celsius=$((20 + i))
    hex_celsius=$(printf '%08x' $celsius)
    hex_seq=$(printf '%08x' $i)
    FRAME="00000000000000001a484de120e8c57500000008${hex_seq}${hex_celsius}"
    curl -s -X POST http://127.0.0.1:9092/publish -d "$FRAME"
    sleep 0.1
done
```

### Result

```
ok
ok
ok
ok
ok
```

## Receiving events

### Poll all events

```sh
curl -s http://127.0.0.1:9092/poll?after=0
```

Response format (first line = total count, then `seq frame_hex` per line):

```
5
1 00000000000000001a484de120e8c575000000080000000100000015
2 00000000000000001a484de120e8c575000000080000000200000016
3 00000000000000001a484de120e8c575000000080000000300000017
4 00000000000000001a484de120e8c575000000080000000400000018
5 00000000000000001a484de120e8c575000000080000000500000019
```

### Poll with cursor (incremental consumption)

```sh
# Get events after #3
curl -s http://127.0.0.1:9092/poll?after=3
# Returns events 4 and 5 only
```

```sh
# Get events after #5 (none)
curl -s http://127.0.0.1:9092/poll?after=5
# Returns: "5" (count only, no frames)
```

### Decoding events

```sh
curl -s http://127.0.0.1:9092/poll?after=0 | while read seq_num frame_hex; do
    payload=$(echo "$frame_hex" | cut -c41-56)
    seq_hex=$(echo "$payload" | cut -c1-8)
    celsius_hex=$(echo "$payload" | cut -c9-16)
    seq_dec=$(printf '%d' "0x$seq_hex")
    celsius_dec=$(printf '%d' "0x$celsius_hex")
    fahrenheit=$((celsius_dec * 9 / 5 + 32))
    echo "Event #$seq_num: seq=$seq_dec, celsius=${celsius_dec}C → ${fahrenheit}F"
done
```

### Decoded output

```
Event #1: seq=1, celsius=21C → 69F
Event #2: seq=2, celsius=22C → 71F
Event #3: seq=3, celsius=23C → 73F
Event #4: seq=4, celsius=24C → 75F
Event #5: seq=5, celsius=25C → 77F
```

This matches the subscriber's Fahrenheit conversion exactly:
- 21°C → 69°F ✓
- 22°C → 71°F ✓
- 23°C → 73°F ✓
- 24°C → 75°F ✓
- 25°C → 77°F ✓

## Error handling

| Test case | curl command | HTTP status | Response body |
|-----------|-------------|-------------|---------------|
| Invalid hex | `-d "ZZZZ"` | 400 | `invalid frame` |
| Truncated frame | `-d "0000000000000000"` | 400 | `invalid frame` |
| Unknown route | `GET /unknown` | 400 | `unknown route` |
| Invalid cursor | `GET /poll?after=abc` | 400 | `invalid cursor` |

### Verification

```sh
curl -s -w " HTTP %{http_code}" -X POST http://127.0.0.1:9092/publish -d "ZZZZ"
# → "invalid frame HTTP 400"

curl -s -w " HTTP %{http_code}" -X POST http://127.0.0.1:9092/publish -d "0000000000000000"
# → "invalid frame HTTP 400"

curl -s -w " HTTP %{http_code}" -X GET http://127.0.0.1:9092/unknown
# → "unknown route HTTP 400"

curl -s -w " HTTP %{http_code}" http://127.0.0.1:9092/poll?after=abc
# → "invalid cursor HTTP 400"
```

## Comparison: curl vs official publisher

| Aspect | curl | Official publisher |
|--------|------|--------------------|
| Protocol | Direct hex frames | tobus event bus |
| Event type ID | Hardcoded `1a484de120e8c575` | Auto-registered at runtime |
| Payload encoding | Manual (big-endian hex) | Automatic (BinaryEventTraits) |
| Discovery | None | Event name → type ID mapping |
| Use case | Testing, debugging, manual API | Production usage |

The curl approach bypasses the tobus event registration system and publishes raw frames directly. This is useful for:
- Testing the HTTP transport layer in isolation
- Debugging frame encoding/decoding
- Simulating external publishers
- Integration testing with non-C++ clients

## Server queue behavior

- The server maintains an in-memory queue of up to **4096 frames** (`max_queue_size`)
- Oldest frames are evicted when the queue is full
- The cursor increments monotonically and wraps around via `sequence - queue_size`
- Polling with `after=0` always returns the full queue content (not just new events)
