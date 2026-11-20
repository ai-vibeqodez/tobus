# HTTP relay: three independent processes

Этот пример показывает HTTP transport в отдельном процессе relay-сервера и двух клиентах:

| Бинарник | Роль |
|---|---|
| `tobus_example_http_relay_server` | принимает `POST /publish` и обслуживает `GET /poll` |
| `tobus_example_http_relay_publisher` | публикует пять событий `TemperatureReading` |
| `tobus_example_http_relay_subscriber` | получает события, печатает их и конвертирует Celsius в Fahrenheit |

## Запуск

Соберите примеры с HTTP:

```sh
cmake -S . -B build -DTOBUS_ENABLE_HTTP=ON -DTOBUS_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

В трех терминалах запустите программы в таком порядке:

```sh
./build/examples/http_relay/tobus_example_http_relay_server
./build/examples/http_relay/tobus_example_http_relay_subscriber
./build/examples/http_relay/tobus_example_http_relay_publisher
```

Endpoint можно передать первым аргументом каждому процессу. Сервер ожидает `host:port`, клиенты — `http://host:port`:

```sh
./build/examples/http_relay/tobus_example_http_relay_server 0.0.0.0:9090
./build/examples/http_relay/tobus_example_http_relay_subscriber http://127.0.0.1:9090
./build/examples/http_relay/tobus_example_http_relay_publisher http://127.0.0.1:9090
```

Клиенты не знают друг о друге: `publisher` публикует события, а `subscriber` подписывается на них и выполняет свою обработку. Relay хранит события в памяти; запускать его следует в доверенной сети.

## Структура

```
http_relay/
├── CMakeLists.txt
├── include/http_relay/events.hpp
└── src/
    ├── server.cpp
    ├── publisher.cpp
    └── subscriber.cpp
```
