cmake_minimum_required(VERSION 3.25)

project(cpp_dep_bridge
  VERSION 0.1.0
  LANGUAGES CXX
)

# ---- Options
option(DEPB_ENABLE_WERROR        "Treat warnings as errors" OFF)
option(DEPB_ENABLE_SANITIZERS    "Enable address/undefined sanitizers" OFF)
option(DEPB_ENABLE_LTO           "Enable link-time optimization" OFF)
option(DEPB_BUILD_TESTS          "Build tests" ON)

# ---- C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Export compile_commands for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Default build type for single-config generators
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "" FORCE)
endif()

# ---- Project-wide warnings (target-based)
add_library(depbridge_warnings INTERFACE)

if(MSVC)
  target_compile_options(depbridge_warnings INTERFACE
    /W4
    /permissive-
    /Zc:__cplusplus
  )
  if(DEPB_ENABLE_WERROR)
    target_compile_options(depbridge_warnings INTERFACE /WX)
  endif()
else()
  target_compile_options(depbridge_warnings INTERFACE
    -Wall -Wextra -Wpedantic
    -Wconversion -Wsign-conversion
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Woverloaded-virtual
    -Wdouble-promotion
    -Wformat=2
  )
  if(DEPB_ENABLE_WERROR)
    target_compile_options(depbridge_warnings INTERFACE -Werror)
  endif()
endif()

# ---- Sanitizers (best-effort, clang/gcc)
add_library(depbridge_sanitizers INTERFACE)
if(DEPB_ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(depbridge_sanitizers INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(depbridge_sanitizers INTERFACE -fsanitize=address,undefined)
endif()

# ---- LTO (IPO)
include(CheckIPOSupported)
if(DEPB_ENABLE_LTO)
  check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
  if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  else()
    message(WARNING "IPO/LTO not supported: ${ipo_error}")
  endif()
endif()

# ---- Dependencies (keep minimal)
# Prefer package managers (vcpkg/conan) to provide these.
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
# Optional: CLI11
find_package(CLI11 CONFIG QUIET)

# ---- Core library
add_library(depbridge_core)
add_library(depbridge::core ALIAS depbridge_core)

target_sources(depbridge_core
  PRIVATE
    src/model/ids.cpp
    src/model/normalize.cpp
    src/cmake/file_api_reader.cpp
    src/cmake/target_graph_builder.cpp
    src/sbom/cyclonedx_writer.cpp
    src/github/dependency_submission.cpp
  PUBLIC
    include/model/types.hpp
    include/model/ids.hpp
    include/model/normalize.hpp
    include/cmake/file_api_reader.hpp
    include/cmake/target_graph_builder.hpp
    include/sbom/cyclonedx_writer.hpp
    include/github/dependency_submission.hpp
)

target_include_directories(depbridge_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(depbridge_core
  PUBLIC
    depbridge_warnings
    depbridge_sanitizers
    fmt::fmt
    nlohmann_json::nlohmann_json
)

# ---- CLI executable
add_executable(depbridge)
target_sources(depbridge PRIVATE src/cli/main.cpp)

if(TARGET CLI11::CLI11)
  target_link_libraries(depbridge PRIVATE CLI11::CLI11)
  target_compile_definitions(depbridge PRIVATE DEPB_HAS_CLI11=1)
else()
  # You can still implement a tiny argument parser yourself for MVP
  target_compile_definitions(depbridge PRIVATE DEPB_HAS_CLI11=0)
endif()

target_link_libraries(depbridge PRIVATE depbridge::core)

# ---- Testing
include(CTest)
if(DEPB_BUILD_TESTS AND BUILD_TESTING)
  enable_testing()
  # Choose one framework later (doctest/catch2/gtest). For now just keep a placeholder.
  add_executable(depbridge_smoke tests/smoke.cpp)
  target_link_libraries(depbridge_smoke PRIVATE depbridge::core)
  add_test(NAME smoke COMMAND depbridge_smoke)
endif()
