# Consumer #2: tobus installed to a prefix and found with find_package (its aw_tpl dependency comes with it).
# Configure with -DCMAKE_PREFIX_PATH=<install prefix>.
cmake_minimum_required(VERSION 3.23)
project(tobus_consumer_find_package LANGUAGES CXX)

find_package(tobus 0.1 CONFIG REQUIRED)

add_executable(consumer ../main.cpp)
target_link_libraries(consumer PRIVATE aw::tobus)

enable_testing()
add_test(NAME consumer COMMAND consumer)
