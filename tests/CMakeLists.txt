cmake_minimum_required(VERSION 3.22)

project(numsim_cas_test)

enable_testing()
include(CTest)

include(FetchContent)

#google test
find_package(googletest QUIET)
if(!GOOGLETEST_IS_DETECTED)
	FetchContent_Declare(
	  googletest
	#  # Specify the commit you depend on and update it regularly.
	  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
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

target_sources(numsim_cas_test
  PRIVATE
    cas_test_helpers.h
    ScalarEvaluatorTest.h
    TensorToScalarDifferentiationTest.h
    ScalarDifferentiationTest.h
    TensorProjectorDifferentiationTest.h
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE  -ftime-report -Wall -Wuninitialized -Werror=uninitialized -Wpedantic -Werror -Wextra -pedantic -Wno-error=comment -Wno-error=overloaded-virtual -Wno-error=pessimizing-move)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /WX /bigobj)
endif()
