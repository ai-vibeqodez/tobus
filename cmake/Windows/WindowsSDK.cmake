# aw_find_winsdk_tool()
#
# Search the Windows SDK for a program or library by name, filtering
# by architecture (e.g. x64, x86, arm64).
#
# Usage:
#   include(cmake/Windows/WindowsSDK)
#   aw_find_winsdk_tool(VAR signtool.exe PROGRAM x64)
#   # Result: ${VAR} contains the full path or is empty
#
# Parameters:
#   VAR         - output variable name (set in parent scope)
#   TOOL_NAME   - name of the tool/file to find (e.g. "signtool.exe", "kernel32.lib")
#   TYPE        - PROGRAM or LIBRARY
#   PLATFORM    - architecture filter (default: x64)
#
# Only works on Windows; silently returns on other platforms.

function(aw_find_winsdk_tool)
    if(NOT WIN32)
        return()
    endif()

    set(_oneValue VAR TOOL_NAME TYPE PLATFORM)
    set(_multiValue)
    cmake_parse_arguments(_SDK "${_multiValue}" "${_oneValue}" "${_multiValue}" ${ARGN})

    if(NOT _SDK_VAR)
        message(FATAL_ERROR "aw_find_winsdk_tool() requires VAR argument")
    endif()
    if(NOT _SDK_TOOL_NAME)
        message(FATAL_ERROR "aw_find_winsdk_tool() requires TOOL_NAME argument")
    endif()
    if(NOT _SDK_TYPE)
        set(_SDK_TYPE PROGRAM)
    endif()
    if(NOT _SDK_PLATFORM)
        set(_SDK_PLATFORM x64)
    endif()

    # Determine search subdirectory based on type
    if(_SDK_TYPE STREQUAL "PROGRAM")
        set(SEARCH_SUBDIR "bin")
    else()
        set(SEARCH_SUBDIR "Lib")
    endif()

    set(_found_paths "")

    # Search in common Windows Kit locations
    foreach(_base
            "C:/Program Files (x86)/Windows Kits/*"
            "C:/Windows Kits/*"
            "C:/Program Files/Windows Kits/*"
    )
        file(GLOB _sdk_roots "${_base}")
        foreach(_sdk_root ${_sdk_roots})
            if(IS_DIRECTORY "${_sdk_root}")
                file(GLOB_RECURSE _matched "${_sdk_root}/${SEARCH_SUBDIR}/**/${_SDK_TOOL_NAME}")
                foreach(_match ${_matched})
                    get_filename_component(_item_dir "${_match}" DIRECTORY)
                    # Check if the platform is in the path as a directory component
                    if("${_item_dir}" MATCHES "[/\\]${_SDK_PLATFORM}([/\\]|$)")
                        list(APPEND _found_paths "${_item_dir}")
                    endif()
                endforeach()
            endif()
        endforeach()
    endforeach()

    list(REMOVE_DUPLICATES _found_paths)

    if(_SDK_TYPE STREQUAL "PROGRAM")
        find_program(_result
            NAMES ${_SDK_TOOL_NAME}
            HINTS ${_found_paths}
            PATHS ${_found_paths}
        )
    else()
        find_library(_result
            NAMES ${_SDK_TOOL_NAME}
            HINTS ${_found_paths}
            PATHS ${_found_paths}
        )
    endif()

    set(${_SDK_VAR} ${_result} PARENT_SCOPE)
endfunction()
