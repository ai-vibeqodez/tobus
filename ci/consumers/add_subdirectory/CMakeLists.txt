cmake_minimum_required(VERSION 3.25)
project(consumer_add_subdirectory CXX)

# add_subdirectory pattern: tobus is adjacent directory
if(NOT TOBUS_SOURCE_DIR)
    set(TOBUS_SOURCE_DIR ".")
endif()

add_subdirectory("${TOBUS_SOURCE_DIR}" tobus.build)

# Create simple consumer executable
add_executable(consumer_app main.cpp)
target_link_libraries(consumer_app PRIVATE aw::tobus aw::tpl)
target_compile_features(consumer_app PRIVATE cxx_std_23)

enable_testing()
add_test(NAME consumer_app COMMAND consumer_app)
