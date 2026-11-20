# HTTP relay example: one server and two independent clients.

find_package(Threads REQUIRED)

add_executable(tobus_example_http_relay_server src/server.cpp)
target_include_directories(tobus_example_http_relay_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(tobus_example_http_relay_server PRIVATE aw::tobus Threads::Threads)
target_compile_options(tobus_example_http_relay_server PRIVATE ${AW_WARNINGS} ${AW_STRICT_WARNINGS})

add_executable(tobus_example_http_relay_publisher src/publisher.cpp)
target_include_directories(tobus_example_http_relay_publisher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(tobus_example_http_relay_publisher PRIVATE aw::tobus Threads::Threads)
target_compile_options(tobus_example_http_relay_publisher PRIVATE ${AW_WARNINGS} ${AW_STRICT_WARNINGS})

add_executable(tobus_example_http_relay_subscriber src/subscriber.cpp)
target_include_directories(tobus_example_http_relay_subscriber PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(tobus_example_http_relay_subscriber PRIVATE aw::tobus Threads::Threads)
target_compile_options(tobus_example_http_relay_subscriber PRIVATE ${AW_WARNINGS} ${AW_STRICT_WARNINGS})
