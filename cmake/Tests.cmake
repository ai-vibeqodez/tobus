# aw_setup_tests()
#
# Top-level CMakeLists.txt: include CTest and add tests/ subdirectory if present.
#
# Usage:
#   include(cmake/Tests)
#   aw_setup_tests()

function(aw_setup_tests)
    if(PROJECT_IS_TOP_LEVEL AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        include(CTest)
        if(BUILD_TESTING)
            add_subdirectory(tests)
        endif()
    endif()
endfunction()

# aw_test_target()
#
# tests/CMakeLists.txt: declare a test executable with Catch2.
#
# Usage:
#   include(cmake/Tests)
#   aw_test_target(NAME MyLib SOURCES a_tests.cpp b_tests.cpp LINK_TARGET aw::MyLib)
#   aw_test_target(NAME MyLib TESTS_SOURCE mylib_tests.cpp LINK_TARGET aw::MyLib)   # single file, still supported
#
# Link target can be:
#   - core variant:  aw::MyLib::MyLib_core
#   - main lib:      aw::MyLib
#   - shared lib:    aw::MyLib

function(aw_test_target)
    set(_oneValue NAME TESTS_SOURCE LINK_TARGET)
    set(_multiValue SOURCES)
    cmake_parse_arguments(_TEST "" "${_oneValue}" "${_multiValue}" ${ARGN})

    if(NOT _TEST_NAME)
        message(FATAL_ERROR "aw_test_target() requires NAME argument")
    endif()
    list(APPEND _TEST_SOURCES ${_TEST_TESTS_SOURCE})
    if(NOT _TEST_SOURCES)
        message(FATAL_ERROR "aw_test_target() requires SOURCES (or TESTS_SOURCE) argument")
    endif()
    if(NOT _TEST_LINK_TARGET)
        message(FATAL_ERROR "aw_test_target() requires LINK_TARGET argument")
    endif()

    # --- Catch2 v3 ---
    include(FetchContent)
    find_package(Catch2 3 QUIET)
    if(NOT Catch2_FOUND)
        FetchContent_Declare(Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG        v3.7.1
            GIT_SHALLOW    TRUE
            SYSTEM          TRUE)
        FetchContent_MakeAvailable(Catch2)
        list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
    endif()
    include(Catch)

    # --- Test executable ---
    add_executable(${_TEST_NAME}_tests ${_TEST_SOURCES})
    target_link_libraries(${_TEST_NAME}_tests PRIVATE ${_TEST_LINK_TARGET} Catch2::Catch2WithMain)
    target_compile_options(${_TEST_NAME}_tests PRIVATE ${AW_WARNINGS})
    catch_discover_tests(${_TEST_NAME}_tests)
endfunction()
