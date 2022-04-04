file(
    RELATIVE_PATH relative_path
    "/${search_INSTALL_CMAKEDIR}"
    "/${CMAKE_INSTALL_BINDIR}/${search_NAME}"
)

get_filename_component(prefix "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
set(config_dir "${prefix}/${search_INSTALL_CMAKEDIR}")
set(config_file "${config_dir}/searchConfig.cmake")

message(STATUS "Installing: ${config_file}")
file(WRITE "${config_file}" "\
get_filename_component(
    _search_executable
    \"\${CMAKE_CURRENT_LIST_DIR}/${relative_path}\"
    ABSOLUTE
)
set(
    SEARCH_EXECUTABLE \"\${_search_executable}\"
    CACHE FILEPATH \"Path to the search executable\"
)
")
list(APPEND CMAKE_INSTALL_MANIFEST_FILES "${config_file}")
