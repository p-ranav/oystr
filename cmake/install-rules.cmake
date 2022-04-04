include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package search)

install(
    TARGETS search_exe
    RUNTIME COMPONENT search_Runtime
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    search_INSTALL_CMAKEDIR "${CMAKE_INSTALL_DATADIR}/${package}"
    CACHE PATH "CMake package config location relative to the install prefix"
)
mark_as_advanced(search_INSTALL_CMAKEDIR)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${search_INSTALL_CMAKEDIR}"
    COMPONENT search_Development
)

# Export variables for the install script to use
install(CODE "
set(search_NAME [[$<TARGET_FILE_NAME:search_exe>]])
set(search_INSTALL_CMAKEDIR [[${search_INSTALL_CMAKEDIR}]])
set(CMAKE_INSTALL_BINDIR [[${CMAKE_INSTALL_BINDIR}]])
" COMPONENT search_Development)

install(
    SCRIPT cmake/install-script.cmake
    COMPONENT search_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
