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
option(NUMSIM_CAS_BUILD_TESTS "Build NumSim_CAS test suite" ${IS_TOPLEVEL_PROJECT})
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
# v1.1.1 (d7e27f3): includes tmech #29 "Fix inner_product returning zeros for a
# lazy operand needing a basis change" — the prior pin (d03f2c2) silently
# produced a zero rank-4 for a lazy operand feeding inner_product, which surfaced
# as half-correct finite-strain tangents (∂(FᵀF)/∂F) downstream in numsim-codegen.
# tmech's tests/examples stay off — otherwise its `examples/symdiff/
# system_nonlinear_equations` ICEs g++-14 in general_lu_decomposition. The pin
# reads only these TMECH_-prefixed toggles; setting the bare BUILD_TESTS /
# BUILD_EXAMPLES names would clobber the cache of any project embedding us.
set(TMECH_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(TMECH_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TMECH_BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  tmech
  GIT_REPOSITORY https://github.com/petlenz/tmech.git
  GIT_TAG d7e27f30807fbd54dd7a0a0b1665a46da28d1681 # v1.1.1
)
FetchContent_MakeAvailable(tmech)

# Registers the installable-package smoke test as a ctest. The test re-resolves
# our install tree from a throwaway consumer, so a consumer-side
# find_dependency(tmech) must succeed — which we cannot auto-detect, hence the
# opt-in. It does not gate the install rules; NUMSIM_CAS_INSTALL_LIBRARY does.
option(NUMSIM_CAS_ENABLE_PACKAGE_SMOKE_TEST
       "Register the installable-package smoke test. Requires a system tmech findable via find_package(tmech)."
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
    # tmech is a FetchContent build target — usable for our own build but
    # not in any export set, so it can't appear in the installed
    # interface. Link it BUILD-only; the installed package re-attaches the
    # consumer's find_dependency(tmech) target in NumSim_CASConfig.cmake.in.
    $<BUILD_INTERFACE:tmech::tmech>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  VERSION   ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  CXX_EXTENSIONS OFF
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

# Compiler warnings and flags. Per-config flags use generator expressions so
# they also apply under multi-config generators, where CMAKE_BUILD_TYPE is empty.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wpedantic -Wno-comment -Wno-overloaded-virtual
        $<$<CONFIG:Debug>:-Wshadow;-Wfloat-conversion;-Wnon-virtual-dtor;-Wcast-align;-Wunused;-Wnull-dereference;-Wdouble-promotion>
        $<$<CONFIG:Release>:-O3>)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PUBLIC /bigobj)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4 /EHsc /utf-8 /permissive- /Zc:preprocessor
        $<$<CONFIG:Debug>:/sdl>
        $<$<CONFIG:Release>:/O2;/GL>)
    # /LTCG pairs with the PRIVATE /GL above; it must not reach consumers.
    target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/LTCG>)
endif()

# Sanitizers
if(NUMSIM_CAS_SANITIZERS)
    target_compile_options(${PROJECT_NAME} PUBLIC
        -fsanitize=address,undefined -fno-sanitize-recover=undefined
        -fno-omit-frame-pointer)
    target_link_options(${PROJECT_NAME} PUBLIC
        -fsanitize=address,undefined -fno-sanitize-recover=undefined)
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
    # PEGTL 3.2.8 has unconditional install() rules and no option to disable
    # them, so without EXCLUDE_FROM_ALL its headers, docs and CMake package land
    # in our install prefix and can shadow a consumer's own pegtl.
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.28)
      FetchContent_Declare(
        pegtl
        GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
        GIT_TAG        3.2.8
        EXCLUDE_FROM_ALL
      )
    else()
      FetchContent_Declare(
        pegtl
        GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
        GIT_TAG        3.2.8
      )
    endif()
    set(PEGTL_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(PEGTL_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
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

  # pegtl is used only by the parser's .cpp files; no installed header includes
  # it and it is header-only, so a consumer needs nothing from it. Link it
  # BUILD-only so it never enters the export set — a FetchContent'd pegtl is not
  # exportable, and $<LINK_ONLY:taocpp::pegtl> would otherwise be baked into the
  # installed package.
  target_link_libraries(NumSim_CAS_Parser
    PUBLIC NumSim_CAS::NumSim_CAS
    PRIVATE $<BUILD_INTERFACE:taocpp::pegtl>
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
      -fsanitize=address,undefined -fno-sanitize-recover=undefined
      -fno-omit-frame-pointer)
    target_link_options(NumSim_CAS_Parser PUBLIC
      -fsanitize=address,undefined -fno-sanitize-recover=undefined)
  endif()

  if(WIN32)
    set_target_properties(NumSim_CAS_Parser PROPERTIES
      WINDOWS_EXPORT_ALL_SYMBOLS ON)
  endif()
endif()

# -----------------------------
# Install
# -----------------------------
# We always build against the pinned FetchContent tmech, which installs
# nothing, so the installed package's find_dependency(tmech) resolves against
# whatever tmech the CONSUMER has. tmech is linked BUILD-only above, so it never
# enters the export set and install(EXPORT) cannot trip on it.
if(NUMSIM_CAS_INSTALL_LIBRARY)
  message(STATUS
    "NumSim_CAS install rules enabled. The installed package requires a "
    "consumer-findable tmech at or newer than the pinned GIT_TAG; see README.")
  set(NUMSIM_CAS_INSTALL_CMAKEDIR
      "${CMAKE_INSTALL_LIBDIR}/cmake/NumSim_CAS")

  # No INCLUDES DESTINATION here: target_include_directories already sets
  # the $<INSTALL_INTERFACE:> include dir; adding it again duplicates it in
  # the exported target.
  install(TARGETS ${PROJECT_NAME}
    EXPORT NumSim_CASTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  # Install headers preserving directory structure. The parser subtree is
  # installed separately below, and only when its library is built — shipping
  # those headers without the library gives consumers link errors.
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/numsim_cas/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/numsim_cas"
    FILES_MATCHING
      PATTERN "*.h"
      PATTERN "*.hpp"
      PATTERN "*.ipp"
      PATTERN "*.tpp"
      PATTERN "parser" EXCLUDE
  )

  # Parser library + its public headers (must register with the export
  # set BEFORE install(EXPORT) runs below).
  if(NUMSIM_CAS_BUILD_PARSER)
    install(TARGETS NumSim_CAS_Parser
      EXPORT NumSim_CASTargets
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
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
