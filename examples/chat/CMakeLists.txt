# Chat example — independent modules connected by the event bus.

add_executable(tobus_example_chat
    main.cpp
    room.cpp
    moderation.cpp
    notifier.cpp
    stats.cpp
)

target_link_libraries(tobus_example_chat PRIVATE aw::tobus)
target_compile_options(tobus_example_chat PRIVATE ${AW_WARNINGS} ${AW_STRICT_WARNINGS})

# Each module .cpp needs async_sleep from aw::tpl.
find_package(Threads REQUIRED)
target_link_libraries(tobus_example_chat PRIVATE Threads::Threads)
