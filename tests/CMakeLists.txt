cmake_minimum_required(VERSION 3.22)

project(numsim_cas_test)

enable_testing()
include(CTest)

include(FetchContent)

#google test
find_package(GTest QUIET)
if(NOT GTest_FOUND)
	FetchContent_Declare(
	  googletest
	  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
	  DOWNLOAD_EXTRACT_TIMESTAMP true
	)
	# For Windows: Prevent overriding the parent project's compiler/linker settings
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)
macro(add_numsim_cas_test TARGET_NAME)
    add_executable(${TARGET_NAME} ${ARGN})
    target_link_libraries(${TARGET_NAME} PRIVATE gtest NumSim_CAS)
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
    gtest_discover_tests(${TARGET_NAME} DISCOVERY_TIMEOUT 30)
endmacro()

add_numsim_cas_test(numsim_cas_test
    main.cpp
    cas_test_helpers.h
    CoreBugFixTest.h
    LimitVisitorTest.h
    ScalarAssumptionTest.h
    ScalarDifferentiationTest.h
    ScalarEvaluatorTest.h
    ScalarExpressionTest.h
    ScalarSubstitutionTest.h
    TensorDifferentiationTest.h
    TensorEvaluatorTest.h
    TensorExpressionTest.h
    TensorProjectorDifferentiationTest.h
    TensorSpacePropagationTest.h
    TensorSubstitutionTest.h
    TensorToScalarDifferentiationTest.h
    TensorToScalarEvaluatorTest.h
    TensorToScalarExpressionTest.h
    TensorToScalarSubstitutionTest.h
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE  -ftime-report -Wall -Wuninitialized -Werror=uninitialized -Wpedantic -Werror -Wextra -pedantic -Wno-error=comment -Wno-error=overloaded-virtual -Wno-error=pessimizing-move)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /WX /bigobj)
endif()
