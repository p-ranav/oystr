file(
    RELATIVE_PATH relative_path
    "/${oystr_INSTALL_CMAKEDIR}"
    "/${CMAKE_INSTALL_BINDIR}/${oystr_NAME}"
)

get_filename_component(prefix "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
set(config_dir "${prefix}/${oystr_INSTALL_CMAKEDIR}")
set(config_file "${config_dir}/oystrConfig.cmake")

message(STATUS "Installing: ${config_file}")
file(WRITE "${config_file}" "\
get_filename_component(
    _oystr_executable
    \"\${CMAKE_CURRENT_LIST_DIR}/${relative_path}\"
    ABSOLUTE
)
set(
    OYSTR_EXECUTABLE \"\${_oystr_executable}\"
    CACHE FILEPATH \"Path to the oystr executable\"
)
")
list(APPEND CMAKE_INSTALL_MANIFEST_FILES "${config_file}")
