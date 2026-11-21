cmake_minimum_required(VERSION 3.25)
project(consumer_find_package CXX)

# find_package pattern: tobus is installed to CMAKE_PREFIX_PATH
find_package(tobus REQUIRED)

# Create simple consumer executable
add_executable(consumer_app main.cpp)
target_link_libraries(consumer_app PRIVATE aw::tobus)
target_compile_features(consumer_app PRIVATE cxx_std_23)

enable_testing()
add_test(NAME consumer_app COMMAND consumer_app)
