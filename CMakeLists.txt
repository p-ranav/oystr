cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    oystr
    VERSION 0.2.0
    DESCRIPTION "oystr recursively searches directories for a substring."
    HOMEPAGE_URL "https://github.com/p-ranav/oystr"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

# ---- Argparse -------------

set(CMAKE_MODULE_PATH "")
set(CMAKE_LIBRARY_ARCHITECTURE "")

include(FetchContent)

FetchContent_Declare(argparse
  GIT_REPOSITORY https://github.com/p-ranav/argparse.git
  GIT_TAG master
)
FetchContent_MakeAvailable(argparse)

# ---- Fmt ----------------

set(FMT_HEADERS "")

FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG master
)
FetchContent_MakeAvailable(fmt)

# ---- Threads ------------
find_package(Threads REQUIRED)

# ---- Declare library ----

add_library(
    oystr_lib OBJECT
    source/searcher.cpp
    source/sse2_strstr.cpp
)

target_include_directories(
    oystr_lib ${warning_guard}
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
    ${argparse_SOURCE_DIR}/include/argparse
)

target_compile_features(oystr_lib PUBLIC cxx_std_17)

target_link_libraries(oystr_lib PRIVATE fmt::fmt Threads::Threads)

# ---- Declare executable ----

add_executable(oystr_exe source/main.cpp)
add_executable(oystr::exe ALIAS oystr_exe)

set_target_properties(
    oystr_exe PROPERTIES
    OUTPUT_NAME oy
    EXPORT_NAME exe
)

target_compile_features(oystr_exe PRIVATE cxx_std_17)

target_link_libraries(oystr_exe PRIVATE oystr_lib fmt::fmt)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT oystr_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of oystr"
  )
endif()

include(cmake/dev-mode.cmake)
