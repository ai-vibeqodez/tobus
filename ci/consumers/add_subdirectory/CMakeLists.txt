# Consumer #1: tobus as a subdirectory of the parent project (what a monorepo or a git submodule does).
# Configure with -DTOBUS_SOURCE_DIR=<path to the tobus checkout>.
cmake_minimum_required(VERSION 3.23)
project(tobus_consumer_add_subdirectory LANGUAGES CXX)

set(TOBUS_SOURCE_DIR "" CACHE PATH "tobus checkout")
if(NOT TOBUS_SOURCE_DIR)
    message(FATAL_ERROR "pass -DTOBUS_SOURCE_DIR=<path to tobus>")
endif()

add_subdirectory(${TOBUS_SOURCE_DIR} tobus)

add_executable(consumer ../main.cpp)
target_link_libraries(consumer PRIVATE aw::tobus)

# Subproject defaults: no tests/examples/install of tobus itself.
if(TOBUS_BUILD_TESTS OR TOBUS_BUILD_EXAMPLES OR TOBUS_INSTALL)
    message(FATAL_ERROR "tobus must not enable tests/examples/install when it is a subproject")
endif()

enable_testing()
add_test(NAME consumer COMMAND consumer)
