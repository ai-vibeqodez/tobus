# Consumer #3: tobus pulled over the network with FetchContent (exactly what smlq_sfw does).
# Configure with -DTOBUS_GIT_TAG=<branch | tag | commit sha> (default: main) and optionally -DTOBUS_GIT_URL=<url>.
cmake_minimum_required(VERSION 3.23)
project(tobus_consumer_fetch_content LANGUAGES CXX)

set(TOBUS_GIT_URL "https://github.com/ai-warevo/tobus.git" CACHE STRING "Repository to fetch tobus from")
set(TOBUS_GIT_TAG "main" CACHE STRING "Branch, tag or commit to fetch")

include(FetchContent)
FetchContent_Declare(tobus GIT_REPOSITORY ${TOBUS_GIT_URL} GIT_TAG ${TOBUS_GIT_TAG})
FetchContent_MakeAvailable(tobus)

add_executable(consumer ../main.cpp)
target_link_libraries(consumer PRIVATE aw::tobus)

enable_testing()
add_test(NAME consumer COMMAND consumer)
