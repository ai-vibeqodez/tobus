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
        
        # Download Catch2 first so we can patch it if needed
        FetchContent_GetProperties(Catch2)
        if(NOT catch2_POPULATED)
            FetchContent_Populate(Catch2)
            
            # On MSVC, patch Catch2's CMakeLists.txt to disable problematic StringMaker specializations
            if(MSVC)
                file(READ "${catch2_SOURCE_DIR}/CMakeLists.txt" _catch2_cmake)
                string(REPLACE 
                    "option(CATCH_CONFIG_NO_CPP17_STRING_VIEW" 
                    "set(CATCH_CONFIG_NO_CPP17_STRING_VIEW ON CACHE BOOL"
                    _catch2_cmake "${_catch2_cmake}")
                string(REPLACE
                    "option(CATCH_CONFIG_NO_CPP17_BYTE"
                    "set(CATCH_CONFIG_NO_CPP17_BYTE ON CACHE BOOL"
                    _catch2_cmake "${_catch2_cmake}")
                file(WRITE "${catch2_SOURCE_DIR}/CMakeLists.txt" "${_catch2_cmake}")
            endif()
            
            add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
        endif()
        
        list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
    endif()
    include(Catch)

    # --- Test executable ---
    add_executable(${_TEST_NAME}_tests ${_TEST_SOURCES})
    target_link_libraries(${_TEST_NAME}_tests PRIVATE ${_TEST_LINK_TARGET} Catch2::Catch2WithMain)
    target_compile_options(${_TEST_NAME}_tests PRIVATE ${AW_WARNINGS})
    # On MSVC, disable Catch2 C++17 StringMaker specializations for std::string_view and std::byte
    # to avoid LNK2019 unresolved external symbols with Catch2 v3.7.1
    if(MSVC)
        target_compile_definitions(${_TEST_NAME}_tests PRIVATE
            CATCH_CONFIG_NO_CPP17_STRING_VIEW
            CATCH_CONFIG_NO_CPP17_BYTE)
    endif()
    catch_discover_tests(${_TEST_NAME}_tests)
endfunction()
