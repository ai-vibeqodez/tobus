cmake_minimum_required(VERSION 3.23)

# --- Fetch reusable CMake modules from template repository ---
include(FetchContent)

FetchContent_Declare(
    aw_cmake_modules
    GIT_REPOSITORY https://github.com/ai-warevo/template_cpp
    GIT_TAG main
    GIT_SHALLOW ON
)

# For local development: set FETCHCONTENT_SOURCE_DIR_AW_CMAKE_MODULES to ../template_cpp
FetchContent_MakeAvailable(aw_cmake_modules)

# Add remote cmake directory to module search path
list(APPEND CMAKE_MODULE_PATH "${aw_cmake_modules_SOURCE_DIR}/cmake")

# --- Project configuration (all from remote) ---
include(ProjectConfig)

aw_project_config(NAME tobus NS aw::tobus)
project(${AW_PROJECT_NAME} VERSION ${AW_PROJECT_VERSION} LANGUAGES CXX)
aw_configure_warnings()

# clang-tidy (CI) reads compile_commands.json. aw_project_config() sets this too, but only in its own function scope.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Additional CMake modules (must come after project()) ---
include(Compiler)

# AW_WARNINGS is also applied directly to examples and tests, outside aw_configure_compiler().

# Top-level builds get tests and examples by default; consumers (FetchContent / add_subdirectory) do not.
if(PROJECT_IS_TOP_LEVEL)
    set(TOBUS_IS_TOP_LEVEL ON)
else()
    set(TOBUS_IS_TOP_LEVEL OFF)
endif()

option(TOBUS_BUILD_TESTS "Build tobus unit tests" ${TOBUS_IS_TOP_LEVEL})
option(TOBUS_BUILD_EXAMPLES "Build tobus examples" ${TOBUS_IS_TOP_LEVEL})
option(TOBUS_INSTALL "Generate install rules and package config for aw::tobus" ${TOBUS_IS_TOP_LEVEL})
option(TOBUS_WARNINGS_AS_ERRORS "Build tobus with strict warnings treated as errors" OFF)
option(AW_ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)
set(AW_SANITIZE "none" CACHE STRING "Sanitizer to enable: none, address (ASan+UBSan) or thread (TSan)")
set_property(CACHE AW_SANITIZE PROPERTY STRINGS none address thread)

# Transports. InProcessBus is always built; the others are stubs for now and OFF by default.
option(TOBUS_ENABLE_UNIX_SOCKET "Build the UnixSocketBus stub" OFF)
option(TOBUS_ENABLE_TCP_SERVER "Build the TCPServerBus stub" OFF)
option(TOBUS_ENABLE_HTTP "Build the HTTPBus transport" OFF)
option(TOBUS_ENABLE_WEBSOCKET "Build the WebSocketBus stub" OFF)
option(TOBUS_ENABLE_REDIS "Enable the RedisBus stub (header-only)" OFF)
option(TOBUS_ENABLE_ZEROMQ "Enable the ZeroMQBus stub (header-only)" OFF)
option(TOBUS_ENABLE_RABBITMQ "Enable the RabbitMQBus stub (header-only)" OFF)
option(TOBUS_ENABLE_KAFKA "Enable the KafkaBus stub (header-only)" OFF)
option(TOBUS_ENABLE_JSON "Build with nlohmann/json support for JsonEventTraits" OFF)

# --- Sanitizer/coverage validation ---
if(NOT AW_SANITIZE STREQUAL "none")
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "AW_SANITIZE requires GCC or Clang")
    endif()
    if(AW_ENABLE_COVERAGE)
        message(FATAL_ERROR "AW_SANITIZE cannot be combined with AW_ENABLE_COVERAGE")
    endif()
endif()

# --- aw::tpl (coroutines) ---------------------------------------------------------------------------------------
# Skipped when the parent project already provides it. For local development point
# FETCHCONTENT_SOURCE_DIR_AW_TPL at a checkout (see the dev-local preset in CMakeUserPresets.json.example).
if(NOT TARGET aw::tpl)
    include(FetchContent)
    FetchContent_Declare(
        aw_tpl
        GIT_REPOSITORY https://github.com/ai-warevo/tpl.git
        GIT_TAG        v3.1.0
    )
    # tpl is a subproject here: no tests/examples; it must install along with us so that our export set resolves.
    set(AW_TPL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(AW_TPL_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(AW_TPL_INSTALL ${TOBUS_INSTALL} CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(aw_tpl)
endif()

# --- nlohmann/json (optional, only for JsonEventTraits) -----------------------------------------------
if(TOBUS_ENABLE_JSON)
    if(NOT TARGET nlohmann_json::nlohmann_json)
        FetchContent_Declare(
            nlohmann_json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG        v3.11.3
        )
        set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(nlohmann_json)
    endif()
    target_compile_definitions(tobus PUBLIC TOBUS_HAS_JSON=1)
    target_link_libraries(tobus PUBLIC nlohmann_json::nlohmann_json)
endif()

# --- library ----------------------------------------------------------------------------------------------------
add_library(tobus STATIC
    src/errors.cpp
    src/transport_kind.cpp
    src/event_bus_factory.cpp
    src/detail/dispatcher.cpp
    src/detail/dispatcher.hpp
    src/transports/in_process_bus.cpp
    src/transports/framed_bus.cpp
)
add_library(aw::tobus ALIAS tobus)
target_compile_features(tobus PUBLIC cxx_std_23)
# src/ is the library's private include root (detail/dispatcher.hpp); it is not part of the installed or exported interface.
target_include_directories(tobus PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Apply compiler-specific configuration
if(TOBUS_WARNINGS_AS_ERRORS)
    aw_configure_compiler(TARGET tobus STRICT_WARNINGS)
else()
    aw_configure_compiler(TARGET tobus)
endif()

target_link_libraries(tobus PUBLIC aw::tpl)

# Each transport: TOBUS_HAS_<X> is 0/1 for every consumer; the socket-based ones also compile their stub source.
set(_tobus_socket_transports UNIX_SOCKET TCP_SERVER HTTP WEBSOCKET)
set(_tobus_broker_transports REDIS ZEROMQ RABBITMQ KAFKA)
foreach(_t IN LISTS _tobus_socket_transports _tobus_broker_transports)
    if(TOBUS_ENABLE_${_t})
        target_compile_definitions(tobus PUBLIC TOBUS_HAS_${_t}=1)
    else()
        target_compile_definitions(tobus PUBLIC TOBUS_HAS_${_t}=0)
    endif()
endforeach()
if(TOBUS_ENABLE_UNIX_SOCKET)
    target_sources(tobus PRIVATE src/transports/unix_socket_bus.cpp)
endif()
if(TOBUS_ENABLE_TCP_SERVER)
    target_sources(tobus PRIVATE src/transports/tcp_server_bus.cpp)
endif()
if(TOBUS_ENABLE_HTTP)
    target_sources(tobus PRIVATE src/transports/http_bus.cpp)
endif()
if(TOBUS_ENABLE_WEBSOCKET)
    target_sources(tobus PRIVATE src/transports/websocket_bus.cpp)
endif()

target_sources(tobus
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/aw/tobus/errors.hpp
            include/aw/tobus/event_bus.hpp
            include/aw/tobus/event_bus_config.hpp
            include/aw/tobus/event_bus_factory.hpp
            include/aw/tobus/event_handler.hpp
            include/aw/tobus/event_traits.hpp
            include/aw/tobus/detail/binary_event_traits_helpers.hpp
            include/aw/tobus/detail/event_traits_impl.hpp
            include/aw/tobus/event_traits/binary_event_traits.hpp
            include/aw/tobus/event_traits/json_event_traits.hpp
            include/aw/tobus/event_type_id.hpp
            include/aw/tobus/subscription.hpp
            include/aw/tobus/tobus.hpp
            include/aw/tobus/transport_kind.hpp
            include/aw/tobus/transports/framed_bus.hpp
            include/aw/tobus/transports/in_process_bus.hpp
            include/aw/tobus/transports/unix_socket_bus.hpp
            include/aw/tobus/transports/tcp_server_bus.hpp
            include/aw/tobus/transports/http_bus.hpp
            include/aw/tobus/transports/websocket_bus.hpp
            include/aw/tobus/transports/redis_bus.hpp
            include/aw/tobus/transports/zeromq_bus.hpp
            include/aw/tobus/transports/rabbitmq_bus.hpp
            include/aw/tobus/transports/kafka_bus.hpp
)

if(TOBUS_INSTALL)
    include(Library/InstallExport)  # From template_cpp/cmake/Library/
    aw_install_library(TARGET tobus NAME tobus NS aw::tobus DEPENDENCIES "aw_tpl 3.1")
endif()

# Tests and examples exist only if the project was generated with them (the directories are optional).
if(TOBUS_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

if(TOBUS_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    include(CTest)
    add_subdirectory(tests)
endif()
