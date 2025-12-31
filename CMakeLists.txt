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

# ---- Export compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Default build type for single-config generators
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "" FORCE)
endif()

# ---- Warnings
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

# ---- Sanitizers (best-effort)
add_library(depbridge_sanitizers INTERFACE)
if(DEPB_ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(depbridge_sanitizers INTERFACE
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
  )
  target_link_options(depbridge_sanitizers INTERFACE
    -fsanitize=address,undefined
  )
endif()

# ---- LTO / IPO
include(CheckIPOSupported)
if(DEPB_ENABLE_LTO)
  check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
  if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  else()
    message(WARNING "IPO/LTO not supported: ${ipo_error}")
  endif()
endif()

# ---- Dependencies
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CLI11 CONFIG QUIET)

# ---- Core library
add_library(depbridge_core)
add_library(depbridge::core ALIAS depbridge_core)

target_sources(depbridge_core
  PRIVATE
    # Phase 3–4
    src/model/ids.cpp
    src/model/normalize.cpp

    # Phase 5
    src/sbom/cyclonedx_writer.cpp

    # Phase 6
    src/ingest/ingest.cpp
    src/ingest/cmake/file_api_ingestor.cpp

  PUBLIC
    # Model
    include/depbridge/model/types.hpp
    include/depbridge/model/ids.hpp
    include/depbridge/model/normalize.hpp

    # SBOM
    include/depbridge/sbom/cyclonedx_writer.hpp

    # Ingestion
    include/depbridge/ingest/ingest.hpp
    include/depbridge/ingest/cmake/file_api_ingestor.hpp
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
  target_compile_definitions(depbridge PRIVATE DEPB_HAS_CLI11=0)
endif()

target_link_libraries(depbridge PRIVATE depbridge::core)

# ---- Tests
include(CTest)
if(DEPB_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
