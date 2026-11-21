cmake_minimum_required(VERSION 3.25)
project(consumer_fetch_content CXX)

include(FetchContent)

# FetchContent pattern: fetch tobus from Git repository
# CI workflow provides TOBUS_GIT_URL and TOBUS_GIT_TAG
set(TOBUS_GIT_URL "" CACHE STRING "Git URL for tobus")
set(TOBUS_GIT_TAG "" CACHE STRING "Git tag/commit for tobus")

if(NOT TOBUS_GIT_URL OR NOT TOBUS_GIT_TAG)
    message(FATAL_ERROR "TOBUS_GIT_URL and TOBUS_GIT_TAG must be provided")
endif()

FetchContent_Declare(
    tobus
    GIT_REPOSITORY ${TOBUS_GIT_URL}
    GIT_TAG        ${TOBUS_GIT_TAG}
    GIT_SHALLOW    ON
)

set(TOBUS_BUILD_TESTS OFF)
set(TOBUS_BUILD_EXAMPLES OFF)
FetchContent_MakeAvailable(tobus)

# Create simple consumer executable
add_executable(consumer_app main.cpp)
target_link_libraries(consumer_app PRIVATE aw::tobus)
target_compile_features(consumer_app PRIVATE cxx_std_23)

enable_testing()
add_test(NAME consumer_app COMMAND consumer_app)
