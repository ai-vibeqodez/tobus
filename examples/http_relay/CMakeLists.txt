# HTTP relay example: one server and two independent clients.
find_package(Threads REQUIRED)

set(_tobus_example_options)
if(TOBUS_WARNINGS_AS_ERRORS)
    list(APPEND _tobus_example_options STRICT_WARNINGS)
endif()

foreach(role IN ITEMS server publisher subscriber)
    aw_add_executable(TARGET tobus_example_http_relay_${role}
        SOURCES src/${role}.cpp
        INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        LINK_LIBRARIES aw::tobus Threads::Threads
        ${_tobus_example_options})
endforeach()
