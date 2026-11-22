# Chat example — independent modules connected by the event bus.
set(_tobus_example_options)
if(TOBUS_WARNINGS_AS_ERRORS)
    list(APPEND _tobus_example_options STRICT_WARNINGS)
endif()

aw_add_executable(TARGET tobus_example_chat
    SOURCES
        src/main.cpp
        src/room.cpp
        src/moderation.cpp
        src/notifier.cpp
        src/stats.cpp
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include/chat
    LINK_LIBRARIES aw::tobus Threads::Threads
    ${_tobus_example_options})
