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
option(BUILD_EXAMPLES "Build NumSim_CAS examples" OFF)
option(BUILD_TESTS "Build NumSim_CAS test suite" ON)
option(BUILD_BENCHMARK "Build NumSim_CAS benchmark" OFF)
option(NUMSIM_CAS_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

set(INSTALL_GTEST OFF CACHE BOOL "Install GoogleTest" FORCE)

# -----------------------------
# Dependencies: tmech
# -----------------------------
find_package(tmech QUIET)
if(NOT tmech_FOUND)
  FetchContent_Declare(
    tmech
    GIT_REPOSITORY https://github.com/petlenz/tmech.git
    GIT_TAG master
  )
  FetchContent_MakeAvailable(tmech)
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
            -Wshadow -Wconversion -Wnon-virtual-dtor -Wcast-align
            -Wunused -Wnull-dereference -Wdouble-promotion)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4 /EHsc /utf-8 /permissive-)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE /sdl)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${PROJECT_NAME} PRIVATE /O2 /GL)
        target_link_options(${PROJECT_NAME} PRIVATE /LTCG)
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
endif()

# -----------------------------
# Tests
# -----------------------------
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# -----------------------------
# Examples
# -----------------------------
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
