# aw_core_library()
#
# Static library with _core suffix, alias, and compile options.
#
# Usage:
#   include(cmake/CoreLibrary)
#   aw_core_library(NAME MyLib ID mylib)
#
# Sets: MyLib_core (STATIC), aw::MyLib::MyLib_core (ALIAS)

function(aw_core_library)
    set(_oneValue NAME ID)
    set(_multiValue SOURCES LINK_PUBLIC LINK_PRIVATE)
    cmake_parse_arguments(_LIB "" "${_oneValue}" "${_multiValue}" ${ARGN})

    if(NOT _LIB_NAME)
        message(FATAL_ERROR "aw_core_library() requires NAME argument")
    endif()
    if(NOT _LIB_ID)
        message(FATAL_ERROR "aw_core_library() requires ID argument")
    endif()

    if(NOT _LIB_SOURCES)
        # Default: src/<ID>.cpp
        set(_LIB_SOURCES src/${_LIB_ID}.cpp)
    endif()

    add_library(${_LIB_NAME}_core STATIC ${_LIB_SOURCES})
    if(DEFINED _NS)
        add_library(${_NS}::${_LIB_NAME}_core ALIAS ${_LIB_NAME}_core)
    else()
        add_library(${_LIB_NAME}::${_LIB_NAME}_core ALIAS ${_LIB_NAME}_core)
    endif()

    target_include_directories(${_LIB_NAME}_core PUBLIC include)
    target_compile_features(${_LIB_NAME}_core PUBLIC cxx_std_20)
    target_compile_options(${_LIB_NAME}_core PRIVATE ${AW_WARNINGS})

    if(_LIB_LINK_PUBLIC)
        target_link_libraries(${_LIB_NAME}_core PUBLIC ${_LIB_LINK_PUBLIC})
    endif()

    if(_LIB_LINK_PRIVATE)
        target_link_libraries(${_LIB_NAME}_core PRIVATE ${_LIB_LINK_PRIVATE})
    endif()
endfunction()
