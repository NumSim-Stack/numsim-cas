cmake_minimum_required(VERSION 3.22)

project(NumSim_CAS
  VERSION 1.0.0
  LANGUAGES CXX
  DESCRIPTION "NumSim_CAS description"
)

# Don't hard-force Debug in multi-config IDEs
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(FetchContent)
include(CMakePackageConfigHelpers)

# -----------------------------
# Options
# -----------------------------
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(IS_TOPLEVEL_PROJECT TRUE)
else()
  set(IS_TOPLEVEL_PROJECT FALSE)
endif()

option(NUMSIM_CAS_INSTALL_LIBRARY "Install NumSim_CAS into default locations" ${IS_TOPLEVEL_PROJECT})
option(NUMSIM_CAS_BUILD_EXAMPLES "Build NumSim_CAS examples" OFF)
option(NUMSIM_CAS_BUILD_TESTS "Build NumSim_CAS test suite" ON)
option(NUMSIM_CAS_BUILD_BENCHMARK "Build NumSim_CAS benchmark" OFF)
option(NUMSIM_CAS_BUILD_PARSER "Build the optional PEGTL-based string parser (issue #214)" OFF)
option(NUMSIM_CAS_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

set(INSTALL_GTEST OFF CACHE BOOL "Install GoogleTest" FORCE)

# -----------------------------
# Dependencies: tmech
# -----------------------------
# Strategy: ALWAYS use the pinned FetchContent copy for our build,
# regardless of whether a system tmech is installed. This eliminates the
# stale-system-tmech bug class (#283/#248: pre-`db5d8aa` installs silently
# produce NaN through the rank-4 `tmech::inv` Voigt path) by construction
# — no marker check needed, no FATAL_ERROR path, no way for a system
# install to leak into the build.
#
# Bump GIT_TAG as a separate maintenance PR.
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  tmech
  GIT_REPOSITORY https://github.com/petlenz/tmech.git
  GIT_TAG d03f2c2
)
FetchContent_MakeAvailable(tmech)
# Restore so our own tests/examples are not affected.
set(BUILD_TESTS ON CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES ON CACHE BOOL "" FORCE)

# Installable-package smoke test (#84) opt-in: the test installs our
# package and re-resolves it from the install tree, which means a
# consumer-side `find_dependency(tmech)` must succeed. With
# FetchContent-always for our build, we cannot auto-detect whether the
# consumer environment has a findable tmech; the smoke test is now
# opt-in via this cache option. Auto-detection is tracked as a
# follow-up to this PR.
option(NUMSIM_CAS_ENABLE_PACKAGE_SMOKE_TEST
       "Run the installable-package smoke test (#84). Requires a system tmech findable via find_package(tmech)."
       OFF)

# tmech::tmech is an ALIAS — resolve to the real target so set_target_properties
# can mark its includes as SYSTEM (suppresses third-party header warnings).
get_target_property(_tmech_real tmech::tmech ALIASED_TARGET)
if(NOT _tmech_real)
  set(_tmech_real tmech::tmech)
endif()
get_target_property(_tmech_inc ${_tmech_real} INTERFACE_INCLUDE_DIRECTORIES)
if(_tmech_inc)
  set_target_properties(${_tmech_real} PROPERTIES
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_tmech_inc}")
endif()

# -----------------------------
# Collect headers and sources
# -----------------------------
file(GLOB_RECURSE NUMSIM_CAS_HEADERS CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/numsim_cas/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/numsim_cas/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/numsim_cas/*.ipp"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/numsim_cas/*.tpp"
)

file(GLOB_RECURSE NUMSIM_CAS_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
)

# Parser sources (in src/numsim_cas/parser/ and include/numsim_cas/parser/)
# live in their own opt-in library. Exclude them from the main library's
# globs so a default NUMSIM_CAS_BUILD_PARSER=OFF build sees no parser code
# at all.
list(FILTER NUMSIM_CAS_SOURCES EXCLUDE REGEX "/parser/")
list(FILTER NUMSIM_CAS_HEADERS EXCLUDE REGEX "/parser/")

# A shared library needs at least one translation unit
if(NUMSIM_CAS_SOURCES STREQUAL "")
  message(FATAL_ERROR
    "No source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src.\n"
    "Create at least one file, e.g. src/numsim_cas.cpp with:\n"
    "  #include <numsim_cas/numsim_cas.h>\n"
  )
endif()

# -----------------------------
# Library target
# -----------------------------
add_library(${PROJECT_NAME}
  ${NUMSIM_CAS_SOURCES}
  ${NUMSIM_CAS_HEADERS}   # for IDE visibility
)

add_library(NumSim_CAS::NumSim_CAS ALIAS ${PROJECT_NAME})

# Nice IDE grouping (optional)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include" PREFIX "Header Files" FILES ${NUMSIM_CAS_HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src"     PREFIX "Source Files" FILES ${NUMSIM_CAS_SOURCES})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    tmech::tmech
)

target_compile_definitions(${PROJECT_NAME}
  PUBLIC
    NUMSIM_CAS_LIBRARY
  PRIVATE
    NUMSIM_CAS_BUILDING_LIBRARY
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  VERSION   ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  CXX_EXTENSIONS OFF
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

# Compiler warnings and flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic -Wno-comment -Wno-overloaded-virtual)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE
            -Wshadow -Wfloat-conversion -Wnon-virtual-dtor -Wcast-align
            -Wunused -Wnull-dereference -Wdouble-promotion)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PUBLIC /bigobj)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4 /EHsc /utf-8 /permissive- /Zc:preprocessor)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE /sdl)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE /O2 /GL)
        target_link_options(${PROJECT_NAME} PUBLIC /LTCG)
    endif()
endif()

# Sanitizers
if(NUMSIM_CAS_SANITIZERS)
    target_compile_options(${PROJECT_NAME} PUBLIC
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${PROJECT_NAME} PUBLIC
        -fsanitize=address,undefined)
endif()

# Optional convenience for Windows
if(WIN32)
  set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# -----------------------------
# Optional parser sub-library (issue #214)
# -----------------------------
# When enabled, builds NumSim_CAS_Parser as a separate static/shared
# library target. Phase 1 shipped error types + symbol_table.
# Phase 2 adds the PEGTL grammar + actions + top-level parse() entry
# points.
if(NUMSIM_CAS_BUILD_PARSER)
  # taocpp/PEGTL — header-only, pulled from upstream when the parser
  # is opted in. Pinned to a stable tag rather than a moving branch
  # so CI is deterministic across reruns.
  find_package(pegtl QUIET)
  if(NOT pegtl_FOUND)
    FetchContent_Declare(
      pegtl
      GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
      GIT_TAG        3.2.8
    )
    set(PEGTL_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(PEGTL_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(PEGTL_INSTALL        OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(pegtl)
    # Suppress warnings from third-party PEGTL headers.
    get_target_property(_pegtl_real taocpp::pegtl ALIASED_TARGET)
    if(NOT _pegtl_real)
      set(_pegtl_real taocpp::pegtl)
    endif()
    get_target_property(_pegtl_inc ${_pegtl_real} INTERFACE_INCLUDE_DIRECTORIES)
    if(_pegtl_inc)
      set_target_properties(${_pegtl_real} PROPERTIES
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_pegtl_inc}")
    endif()
  endif()

  add_library(NumSim_CAS_Parser
    src/numsim_cas/parser/parse_error.cpp
    src/numsim_cas/parser/symbol_table.cpp
    src/numsim_cas/parser/parser.cpp
  )
  add_library(NumSim_CAS::Parser ALIAS NumSim_CAS_Parser)

  target_include_directories(NumSim_CAS_Parser
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  target_link_libraries(NumSim_CAS_Parser
    PUBLIC NumSim_CAS::NumSim_CAS
    PRIVATE taocpp::pegtl
  )

  target_compile_features(NumSim_CAS_Parser PUBLIC cxx_std_23)

  set_target_properties(NumSim_CAS_Parser PROPERTIES
    VERSION   ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_EXTENSIONS OFF
  )

  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(NumSim_CAS_Parser PRIVATE
      -Wall -Wextra -Wpedantic)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(NumSim_CAS_Parser PRIVATE
      /W4 /EHsc /utf-8 /permissive- /Zc:preprocessor)
  endif()

  if(NUMSIM_CAS_SANITIZERS)
    target_compile_options(NumSim_CAS_Parser PUBLIC
      -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(NumSim_CAS_Parser PUBLIC
      -fsanitize=address,undefined)
  endif()

  if(WIN32)
    set_target_properties(NumSim_CAS_Parser PROPERTIES
      WINDOWS_EXPORT_ALL_SYMBOLS ON)
  endif()
endif()

# -----------------------------
# Install
# -----------------------------
# Install support requires a consumer-findable tmech (referenced by
# NumSim_CASConfig.cmake.in via find_dependency(tmech)). The
# FetchContent-always strategy above doesn't install tmech (FetchContent
# doesn't install anything), so install is opt-in via
# NUMSIM_CAS_ENABLE_PACKAGE_SMOKE_TEST — when ON, the user is responsible
# for having tmech installed system-wide for the consumer's
# find_dependency to succeed. Default OFF prevents the
# `install(EXPORT)` from tripping on tmech not being in any export set.
if(NUMSIM_CAS_INSTALL_LIBRARY AND NUMSIM_CAS_ENABLE_PACKAGE_SMOKE_TEST)
  set(NUMSIM_CAS_INSTALL_CMAKEDIR
      "${CMAKE_INSTALL_LIBDIR}/cmake/NumSim_CAS")

  install(TARGETS ${PROJECT_NAME}
    EXPORT NumSim_CASTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  # Install headers preserving directory structure
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/numsim_cas/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/numsim_cas"
    FILES_MATCHING
      PATTERN "*.h"
      PATTERN "*.hpp"
      PATTERN "*.ipp"
      PATTERN "*.tpp"
  )

  # Parser library + its public headers (must register with the export
  # set BEFORE install(EXPORT) runs below).
  if(NUMSIM_CAS_BUILD_PARSER)
    install(TARGETS NumSim_CAS_Parser
      EXPORT NumSim_CASTargets
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/numsim_cas/parser/"
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/numsim_cas/parser"
      FILES_MATCHING PATTERN "*.h"
    )
  endif()

  install(EXPORT NumSim_CASTargets
    NAMESPACE NumSim_CAS::
    DESTINATION "${NUMSIM_CAS_INSTALL_CMAKEDIR}"
  )

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/NumSim_CASConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NumSim_CASConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/NumSim_CASConfig.cmake"
    INSTALL_DESTINATION "${NUMSIM_CAS_INSTALL_CMAKEDIR}"
  )

  install(
    FILES
      "${CMAKE_CURRENT_BINARY_DIR}/NumSim_CASConfig.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/NumSim_CASConfigVersion.cmake"
    DESTINATION "${NUMSIM_CAS_INSTALL_CMAKEDIR}"
  )
elseif(NUMSIM_CAS_INSTALL_LIBRARY)
  # NUMSIM_CAS_INSTALL_LIBRARY=ON but tmech was FetchContent'd (not found
  # as an external package). The install(TARGETS ...) above is skipped
  # entirely — nothing is written to the install prefix. Use WARNING so
  # users who set the option intentionally see the silent regression
  # (STATUS is easily lost in CMake's normal output, especially via IDE
  # configure runs).
  message(WARNING
    "NUMSIM_CAS_INSTALL_LIBRARY is ON, but install was SKIPPED because "
    "tmech is not an installable external package in this build (it was "
    "fetched via FetchContent). To produce an installable NumSim_CAS "
    "package, install tmech first (so find_package(tmech) succeeds), "
    "then reconfigure.")
endif()

# -----------------------------
# Tests
# -----------------------------
if(NUMSIM_CAS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# -----------------------------
# Examples
# -----------------------------
if(NUMSIM_CAS_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
