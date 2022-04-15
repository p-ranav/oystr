include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package oystr)

install(
    TARGETS oystr_exe
    RUNTIME COMPONENT oystr_Runtime
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    oystr_INSTALL_CMAKEDIR "${CMAKE_INSTALL_DATADIR}/${package}"
    CACHE PATH "CMake package config location relative to the install prefix"
)
mark_as_advanced(oystr_INSTALL_CMAKEDIR)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${oystr_INSTALL_CMAKEDIR}"
    COMPONENT oystr_Development
)

# Export variables for the install script to use
install(CODE "
set(oystr_NAME [[$<TARGET_FILE_NAME:oystr_exe>]])
set(oystr_INSTALL_CMAKEDIR [[${oystr_INSTALL_CMAKEDIR}]])
set(CMAKE_INSTALL_BINDIR [[${CMAKE_INSTALL_BINDIR}]])
" COMPONENT oystr_Development)

install(
    SCRIPT cmake/install-script.cmake
    COMPONENT oystr_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
