# 🚌 aw::tobus

> `auto bus = aw::tobus::EventBusFactory::create();` — Быстрая маршрутка для ваших событий: внутри процесса везёт без пересадок и сериализации, а при необходимости легко выходит на межгородские рейсы по сети.

Независимая от транспорта шина событий (event bus) для **C++20** (`aw::tobus`). Модули зависят только от интерфейса `IEventBus`; выбор транспорта под капотом (внутрипроцессная «маршрутка» сегодня; сокеты и брокеры — завтра) определяется исключительно конфигурацией.

Используйте код с осторожностью.
Module --publish--> IEventBus --subscribe--> Module
                      ^
              "какой маршрут?"  in_process / unix_socket / redis / kafka / ...

Зависит только от корутин [`aw::tpl`](https://github.com/ai-warevo/tpl) и стандартной библиотеки C++.

## 💡 Зачем это нужно

Обычно модуль A вызывает модуль B напрямую: A знает про B, включает его заголовок и ждёт ответа. Когда модулей становится много, всё связывается в клубок.

С шиной модули не знают друг друга. Один **публикует событие** («пользователь вошёл», «пришло сообщение»), другие на него **подписаны** и реагируют. Публикующий не знает, сколько у события слушателей и есть ли они вообще. Новый слушатель добавляется без правок в существующем коде.

```
 Auth ──UserJoined──┐
                    ├─► IEventBus ──► Room, Stats, ...   (кто подписан — тот и получит)
 Room ──MessageSent─┘
```

## 🚀 Быстрый старт

```cpp
#include <aw/tobus/tobus.hpp>
using namespace aw::tobus;

struct UserJoined { std::string name; };                  // событие — обычная структура

int main() {
    auto bus = EventBusFactory::create();                 // EventBusConfig{} = in-process

    // Подписка: «когда кто-то опубликует UserJoined — вызови эту функцию».
    // Обработчик — корутина (Task<void>), поэтому внутри можно делать co_await.
    auto sub = bus->subscribe<UserJoined>([](const UserJoined& e) -> Task<void> {
        std::cout << "привет, " << e.name << "\n";
        co_return;
    });                                                   // sub живёт — подписка активна; уничтожили — отписались

    bus->publish(UserJoined{"alice"});                    // fire-and-forget: вернётся сразу, обработчик отработает в фоне
    bus->drain().wait();                                  // дождаться всех начатых обработчиков (при завершении и в тестах)
}
```

## 🧩 Пример

Полный разбор — [`examples/README.md`](examples/README.md).

## ⚙️ Семантика

| Механизм | Поведение |
|---|---|
| `publish<T>(event)` | Никогда не ждёт обработчиков. Событие перемещается в один общий `shared_ptr<const T>`; каждый пассажир-подписчик получает константную ссылку на этот объект. |
| Обработчики (Handlers) | `std::function<Task<void>(const T&)>` — по одной eager-задаче `aw::tpl` на каждую доставку, поэтому обработчики выполняются конкурентно. Событие живёт до завершения задачи, так что обработчик может безопасно уходить в `co_await`. |
| Порядок доставки | Не гарантируется ни между разными обработчиками, ни между двумя событиями для одного обработчика. |
| Ошибки | Исключения из обработчиков передаются в `EventBusConfig::on_error` и никогда не вылетают в отправителя. Если колбэк не задан, ошибка пишется одной строкой в `stderr` (с именем события), а не теряется молча; чтобы заглушить её осознанно, задайте пустой обработчик: `cfg.on_error = [](const std::exception_ptr&) {};`. Колбэк вызывается из потоков пула, возможно параллельно. |
| `Subscription` | Move-only RAII-билет; при разрушении отписывает слушателя (уже запущенные обработчики не отменяются и не ожидаются). Может пережить сам объект шины. |
| `drain()` | Возвращает `Task<void>`, который завершается, когда отработали все начатые доставки, включая каскадные события. Деструктор шины не блокирует поток. Метода `shutdown()` нет: `drain()` — точка тишины, а отмена уже запущенных обработчиков не поддерживается. Сетевые транспорты закрывают соединения в деструкторе; асинхронный `close()` появится вместе с ними как не-чисто-виртуальный метод (по умолчанию — `drain()`), без поломки существующего API. |
| Идентичность типа | `EventTypeId` — 64-битный FNV-1a от `EventTraits<T>::name`. Два разных C++-типа с одним id внутри процесса обнаруживаются при `subscribe` и `publish` (по `std::type_info`) и приводят к `std::logic_error`, так что событие никогда не будет доставлено «не тому» типу. Между процессами по сети идёт только id: коллизия между разными программами локально не обнаружима (вероятность ≈ N²/2⁶⁵; при необходимости транспорт может добавить имя или хэш схемы в заголовок фрейма). |

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

**Санитайзеры и покрытие.** Флаги задаются опциями `aw::tpl` и по `PUBLIC`-зависимости попадают в `tobus` и тесты:

```sh
cmake -S . -B build/asan -DAW_TPL_SANITIZE=address            # ASan + UBSan   (thread → TSan)
cmake -S . -B build/cov  -DAW_TPL_ENABLE_COVERAGE=ON          # gcc --coverage
ctest --test-dir build/asan -R tobus_concurrency --repeat until-fail:30   # многопоточные тесты ([threads]) много раз подряд
```

**CI** (`.github/workflows/`):

| Файл | Что проверяет |
|---|---|
| `ci.yml` | сборка и тесты на PR: clang-18 + libc++ и g++-15, Debug/Release, все транспорты включены, `-Werror` |
| `test.yml` | clang-format, clang-tidy (набор проверок — в `.clang-tidy`), ASan+UBSan, TSan, 30× повтор многопоточных тестов, покрытие (бейдж и отчёт публикуются только из `main`) |
| `consumers.yml` | потребитель проекта: `add_subdirectory`, `find_package` и `FetchContent` с GitHub (закреплён на проверяемом коммите), проекты — в `ci/consumers/` |

Раннеры закреплены на `ubuntu-24.04` (`ubuntu-latest` переезжает на 26.04). g++-15 ставится из PPA `ubuntu-toolchain-r/test`; TSan собирается clang-18 + libstdc++, потому что в libc++ из дистрибутива не инструментированы внутренности `shared_ptr`.

## 📄 Лицензия

MIT. Подробности в файле `LICENSE`.
