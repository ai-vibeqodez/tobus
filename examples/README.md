# Examples

Каждый пример — отдельная директория со своим `CMakeLists.txt`.

| Пример | Описание |
|---|---|
| [chat](chat/README.md) | Событийный чат: несколько независимых модулей, связанных через event bus (транспорт **in_process**) |
| [http_relay](http_relay/README.md) | HTTP relay с отдельными server, publisher и subscriber процессами (требует `TOBUS_ENABLE_HTTP=ON`) |
