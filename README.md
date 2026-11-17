# 🚌 aw::tobus

> `auto bus = aw::tobus::EventBusFactory::create();` — Быстрая маршрутка для ваших событий: внутри процесса везёт без пересадок и сериализации, а при необходимости легко выходит на межгородские рейсы по сети.

Независимая от транспорта шина событий (event bus) для **C++20** (`aw::tobus`). Модули зависят только от интерфейса `IEventBus`; выбор транспорта под капотом (внутрипроцессная «маршрутка» сегодня; сокеты и брокеры — завтра) определяется исключительно конфигурацией.

Используйте код с осторожностью.
Module --publish--> IEventBus --subscribe--> Module
                      ^
              "какой маршрут?"  in_process / unix_socket / redis / kafka / ...

Зависит только от корутин [`aw::tpl`](https://github.com/ai-warevo/tpl) и стандартной библиотеки C++.

## 🚀 Быстрый старт

```cpp
#include <aw/tobus/tobus.hpp>
using namespace aw::tobus;

struct UserJoined { std::string name; };

int main() {
    auto bus = EventBusFactory::create();                 // auto bus = aw::tobus (EventBusConfig{} = in-process)

    // Садимся в маршрутку (оформляем подписку)
    auto sub = bus->subscribe<UserJoined>([](const UserJoined& e) -> Task<void> {
        std::cout << "привет, " << e.name << "\n";
        co_return;
    });

    bus->publish(UserJoined{"alice"});                    // передаём за проезд (fire-and-forget: возврат сразу)
    bus->drain().wait();                                  // конечная остановка (опциональное ожидание при завершении или в тестах)
}
```

## ⚙️ Семантика

| Механизм | Поведение |
|---|---|
| `publish<T>(event)` | Никогда не ждёт обработчиков. Событие перемещается в один общий `shared_ptr<const T>`; каждый пассажир-подписчик получает константную ссылку на этот объект. |
| Обработчики (Handlers) | `std::function<Task<void>(const T&)>` — по одной eager-задаче `aw::tpl` на каждую доставку, поэтому обработчики выполняются конкурентно. Событие живёт до завершения задачи, так что обработчик может безопасно уходить в `co_await`. |
| Порядок доставки | Не гарантируется ни между разными обработчиками, ни между двумя событиями для одного обработчика. |
| Ошибки | Исключения из обработчиков передаются в `EventBusConfig::on_error` (или игнорируются, если колбэк не задан) и никогда не вылетают в отправителя. |
| `Subscription` | Move-only RAII-билет; при разрушении отписывает слушателя (уже запущенные обработчики не отменяются и не ожидаются). Может пережить сам объект шины. |
| `drain()` | Возвращает `Task<void>`, который завершается, когда отработали все начатые доставки, включая каскадные события. Деструктор шины не блокирует поток. |

## 📦 Граница сериализации

`InProcessBus` никогда не сериализует данные: события едут внутри процесса по `shared_ptr` без лишнего копирования. Чтобы пустить событие по сетевому маршруту, специализируйте шаблон `EventTraits` — `tobus` не навязывает формат данных, кодек остаётся за вами:

```cpp
template <> struct aw::tobus::EventTraits<Ping> {
    static constexpr std::string_view name = "myapp.Ping";       // стабильное имя маршрута между процессами
    static std::vector<std::byte> serialize(const Ping&);
    static Ping deserialize(std::span<const std::byte>);
};
```

Сетевые транспорты выбрасывают `NotSerializable` при вызове `subscribe<T>()`, если для `T` не определён кодек (ошибка конфигурации на старте), и передают его в `on_error` при `publish<T>()` в качестве страховки. Локальные подписчики отправителя всегда получают события напрямую; транспорты отбрасывают собственное эхо по `origin id` в заголовке фрейма.

## 🚏 Транспорты (маршруты)

| Транспорт | Статус | CMake-опция |
|---|---|---|
| `InProcessBus` | реализован | включён всегда |
| `UnixSocketBus`, `TCPServerBus`, `HTTPBus`, `WebSocketBus` | заглушка (`// TODO: implement`) | `TOBUS_ENABLE_UNIX_SOCKET`, `_TCP_SERVER`, `_HTTP`, `_WEBSOCKET` |
| `RedisBus`, `ZeroMQBus`, `RabbitMQBus`, `KafkaBus` | header-only заглушка | `TOBUS_ENABLE_REDIS`, `_ZEROMQ`, `_RABBITMQ`, `_KAFKA` |

`FramedBus` задуман как базовый класс для сетевых транспортов (отправка и приём фреймов; контракт описан в заголовочном файле). При реализации новых транспортов используйте `InProcessBus` и `detail::Dispatcher` в качестве эталона.
Попытка создать не скомпилированный транспорт через `EventBusFactory::create` выбросит `TransportNotAvailable` с указанием нужной CMake-опции.

## 🛠 Подключение через CMake

```cmake
include(FetchContent)
FetchContent_Declare(tobus GIT_REPOSITORY https://github.com/ai-warevo/tobus.git GIT_TAG main)
FetchContent_MakeAvailable(tobus)
target_link_libraries(app PRIVATE aw::tobus)
```

Библиотека `aw::tpl` подтягивается автоматически, если таргет ещё не определён в родительском проекте.

| Опция | По умолчанию | Описание |
|---|---|---|
| `TOBUS_BUILD_TESTS` / `TOBUS_BUILD_EXAMPLES` / `TOBUS_INSTALL` | `ON` для корневого проекта, `OFF` как подпроект | Сборка тестов, примеров и правил установки |
| `TOBUS_WARNINGS_AS_ERRORS` | `OFF` | Включает `-Wshadow -Wconversion -Werror` для библиотеки и примеров |
| `TOBUS_ENABLE_<TRANSPORT>` | `OFF` | Включение конкретных транспортов (см. таблицу выше) |

## 💻 Разработка и сборка

```sh
cp CMakeUserPresets.json.example CMakeUserPresets.json     # локальные пресеты (в .gitignore); использует /workspace/tpl
cmake --preset dev-local && cmake --build --preset dev-local && ctest --preset dev-local
cmake --preset all-transports && cmake --build --preset all-transports && ctest --preset all-transports   # clang-18 + libc++
```

**Тулчейн:** C++20, CMake 3.23+, `clang-18` + `libc++` (флаг `-fexperimental-library` добавляется автоматически из `aw::tpl`) либо `GCC 15` / `clang 19` с `libstdc++`.
Структура проекта и соглашения по коду следуют `template-cpp`; история изменений ведётся в `CHANGELOG.md`.

## 📄 Лицензия

MIT. Подробности в файле `LICENSE`.
