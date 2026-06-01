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
# numsim-cas requires tmech with the post-Sep-2025 inverse_wrapper rewrite.
# Older system installs predating this rewrite produce NaN through the rank-4
# Voigt inv path (verified during #248). When find_package locates a system
# tmech we probe for the rewrite marker (inverse_wrapper_base) and error out
# clearly if absent — silently building against a broken tmech is the kind
# of footgun that surfaces as wrong numerical results in production.
#
# Workaround for developers with stale system installs:
#   cmake -DCMAKE_DISABLE_FIND_PACKAGE_tmech=TRUE ...
# forces the FetchContent path below.
find_package(tmech QUIET)
if(tmech_FOUND)
  # Resolve include dir of the found target.
  get_target_property(_tmech_real tmech::tmech ALIASED_TARGET)
  if(NOT _tmech_real)
    set(_tmech_real tmech::tmech)
  endif()
  get_target_property(_tmech_inc ${_tmech_real} INTERFACE_INCLUDE_DIRECTORIES)
  if(NOT _tmech_inc)
    # Some find_package setups expose the include dir via a variable.
    set(_tmech_inc "${tmech_INCLUDE_DIRS}")
  endif()
  # Probe for the rewrite marker. Use file content rather than try_compile to
  # avoid template-instantiation noise — the symbol exists or it doesn't.
  set(_tmech_inv_meat "${_tmech_inc}/tmech/tensor/inverse_wrapper_meat.h")
  set(_tmech_has_rewrite OFF)
  if(EXISTS "${_tmech_inv_meat}")
    file(READ "${_tmech_inv_meat}" _tmech_inv_meat_content)
    string(FIND "${_tmech_inv_meat_content}" "inverse_wrapper_base"
           _tmech_marker_pos)
    if(NOT _tmech_marker_pos EQUAL -1)
      set(_tmech_has_rewrite ON)
    endif()
  endif()
  if(NOT _tmech_has_rewrite)
    message(FATAL_ERROR
      "Found a system tmech installation at '${_tmech_inc}' that predates "
      "the inverse_wrapper rewrite (no inverse_wrapper_base symbol). "
      "Rank-4 inv() will silently produce NaN with this version "
      "(see numsim-cas #248). Required: tmech master at commit db5d8aa "
      "or later. Either update the system install, or reconfigure with "
      "  -DCMAKE_DISABLE_FIND_PACKAGE_tmech=TRUE\n"
      "to force the FetchContent path.")
  endif()
endif()
if(NOT tmech_FOUND)
  # Disable tmech's own tests and examples (it uses BUILD_TESTS/BUILD_EXAMPLES)
  set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    tmech
    GIT_REPOSITORY https://github.com/petlenz/tmech.git
    # Pinned to the master commit known to have the inverse_wrapper rewrite.
    # Bump as a separate maintenance PR; see #248 for the rationale.
    GIT_TAG db5d8aa
  )
  FetchContent_MakeAvailable(tmech)
  # Restore so downstream (our own tests/examples) is not affected
  set(BUILD_TESTS ON CACHE BOOL "" FORCE)
  set(BUILD_EXAMPLES ON CACHE BOOL "" FORCE)
  # Suppress warnings from third-party tmech headers.
  # tmech::tmech is an ALIAS — resolve to real target name for set_target_properties.
  get_target_property(_tmech_real tmech::tmech ALIASED_TARGET)
  if(NOT _tmech_real)
    set(_tmech_real tmech::tmech)
  endif()
  get_target_property(_tmech_inc ${_tmech_real} INTERFACE_INCLUDE_DIRECTORIES)
  if(_tmech_inc)
    set_target_properties(${_tmech_real} PROPERTIES
      INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_tmech_inc}")
  endif()
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
if(NUMSIM_CAS_INSTALL_LIBRARY)
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

  if(tmech_FOUND)
    install(EXPORT NumSim_CASTargets
      NAMESPACE NumSim_CAS::
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NumSim_CAS"
    )
  endif()

  # Parser library + its public headers (conditional on the option).
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
