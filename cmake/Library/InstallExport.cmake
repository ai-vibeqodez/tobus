# aw_install_library()
#
# Install rules + export targets + CMake package config files.
#
# Usage:
#   include(cmake/Library/InstallExport)
#   aw_install_library(NAME MyLib NS aw:: EXPORT_NAME mylib)
#   aw_install_library(NAME tpl NS aw::tpl DEPENDENCIES Threads "OtherPkg 2.0")
#
# NS may be a prefix ("aw::") or, when it does not end in "::", the full alias of the library
# ("aw::tpl"): consumers then link `aw::tpl`, and the package config is named after NAME.
# DEPENDENCIES: one find_dependency() call per entry is written into the package config.
#
# Requires: GNUInstallDirs, CMakePackageConfigHelpers (included internally)
# Depends on: CMAKE_CURRENT_BINARY_DIR, CMAKE_CURRENT_SOURCE_DIR, PROJECT_VERSION

function(aw_install_library)
    set(_oneValue NAME NS EXPORT_NAME)
    set(_multiValue DEPENDENCIES)
    cmake_parse_arguments(_INST "" "${_oneValue}" "${_multiValue}" ${ARGN})

    if(NOT _INST_NAME)
        message(FATAL_ERROR "aw_install_library() requires NAME argument")
    endif()
    if(NOT _INST_NS)
        message(FATAL_ERROR "aw_install_library() requires NS argument")
    endif()

    # NS without a trailing "::" is the full alias (aw::tpl): split it into prefix "aw::" and exported name "tpl".
    set(_inst_target_name "")
    if(NOT _INST_NS MATCHES "::$")
        if(_INST_NS MATCHES "^(.*::)([^:]+)$")
            set(_INST_NS "${CMAKE_MATCH_1}")
            set(_inst_target_name "${CMAKE_MATCH_2}")
        else()
            set(_inst_target_name "${_INST_NS}")
            set(_INST_NS "")
        endif()
    endif()

    if(NOT _INST_EXPORT_NAME)
        set(_INST_EXPORT_NAME ${_INST_NAME})
    endif()

    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    if(_inst_target_name)
        set_target_properties(${_INST_NAME} PROPERTIES EXPORT_NAME ${_inst_target_name})
    endif()

    # --- Install targets ---
    # CMake 3.23+ supports FILE_SET HEADERS (preferred);
    # for older versions fall back to target_include_directories.
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.23)
        install(TARGETS ${_INST_NAME}
            EXPORT ${_INST_EXPORT_NAME}Targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    else()
        install(TARGETS ${_INST_NAME}
            EXPORT ${_INST_EXPORT_NAME}Targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()

    # --- Install export ---
    install(EXPORT ${_INST_EXPORT_NAME}Targets
        FILE ${_INST_EXPORT_NAME}Targets.cmake
        NAMESPACE ${_INST_NS}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${_INST_NAME}
    )

    # --- Version file ---
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${_INST_EXPORT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    # --- Config file (generated) ---
    set(_inst_config "include(CMakeFindDependencyMacro)\n")
    foreach(_dep IN LISTS _INST_DEPENDENCIES)
        string(APPEND _inst_config "find_dependency(${_dep})\n")
    endforeach()
    string(APPEND _inst_config "include(\"\${CMAKE_CURRENT_LIST_DIR}/${_INST_EXPORT_NAME}Targets.cmake\")\n")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${_INST_EXPORT_NAME}Config.cmake" "${_inst_config}")

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${_INST_EXPORT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${_INST_EXPORT_NAME}ConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${_INST_NAME}
    )
endfunction()
