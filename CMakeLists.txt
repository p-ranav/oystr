cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    search
    VERSION 0.1.0
    DESCRIPTION "Command-line utility for searching plain-text data sets for lines that match a substring."
    HOMEPAGE_URL "https://github.com/p-ranav/search"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    search_lib OBJECT
    source/search.cpp
)

target_include_directories(
    search_lib ${warning_guard}
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(search_lib PUBLIC cxx_std_17)

# ---- Declare executable ----

add_executable(search_exe source/main.cpp)
add_executable(search::exe ALIAS search_exe)

set_target_properties(
    search_exe PROPERTIES
    OUTPUT_NAME search
    EXPORT_NAME exe
)

target_compile_features(search_exe PRIVATE cxx_std_17)

target_link_libraries(search_exe PRIVATE search_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT search_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of search"
  )
endif()

include(cmake/dev-mode.cmake)
