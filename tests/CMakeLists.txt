cmake_minimum_required(VERSION 3.22)

project(numsim_cas_test)

set(CMAKE_CXX_STANDARD 23)

enable_testing()
include(CTest)

#google test
include(FetchContent)
FetchContent_Declare(
  googletest
#  # Specify the commit you depend on and update it regularly.
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
#

include(GoogleTest)
macro(add_numsim_cas_test TARGET_NAME)
    add_executable(${TARGET_NAME} ${ARGN})
    target_link_libraries(${TARGET_NAME} PRIVATE gtest_main NumSim_CAS)
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
    gtest_discover_tests(${TARGET_NAME})
endmacro()

add_numsim_cas_test(numsim_cas_test
    main.cpp
    symTM_test.h
    symTM_print_test.h
    symTM_diff_test.h
    ScalarExpressionTest.h
    TensorToScalarExpressionTest.h
    TensorExpressionTest.h)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE  -Wall -Wpedantic -Werror -Wextra -pedantic -Wno-error=comment)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /WX)
endif()
