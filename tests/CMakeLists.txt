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
    SolveTest.h
    LeviCivitaTest.h
    LimitVisitorTest.h
    NumericalDiffHelpers.h
    NumericalDiffTest.h
    ParserTest.h
    ScalarAssumptionTest.h
    ScalarComparisonTest.h
    ScalarDifferentiationTest.h
    ScalarEvaluatorTest.h
    ScalarExpressionTest.h
    ScalarSubstitutionTest.h
    TensorDifferentiationTest.h
    TensorEvaluatorTest.h
    TensorExpressionTest.h
    TensorInvDiffSymTest.h
    TensorProjectorDifferentiationTest.h
    TensorSpacePropagationTest.h
    TensorSubstitutionTest.h
    TensorToScalarDifferentiationTest.h
    TensorToScalarDivOperatorTest.h
    TensorToScalarMulOperatorTest.h
    TensorToScalarEvaluatorTest.h
    TensorToScalarExpressionTest.h
    TensorToScalarSubstitutionTest.h
    ScalarLatexPrinterTest.h
    TensorLatexPrinterTest.h
    TensorToScalarLatexPrinterTest.h
)

add_numsim_cas_test(numsim_cas_fuzz_test
    FuzzyTensorDiffTest.cpp
    cas_test_helpers.h
)

foreach(target numsim_cas_test numsim_cas_fuzz_test)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Deprecation warnings stay visible but don't fail the build.
        # Step 7 of the SymPy assumption-system redesign marked the
        # legacy assume(holder, tag) and assume_*(holder) helpers as
        # [[deprecated]] in favor of expression_holder::assumption().
        # ~200 existing test call sites use them and would all become
        # errors under -Werror; downgrade just this one warning so the
        # soft-deprecation signal stays present without breaking CI.
        target_compile_options(${target} PRIVATE -Wall -Wuninitialized -Werror=uninitialized -Wpedantic -Werror -Wextra -pedantic -Wno-error=comment -Wno-error=overloaded-virtual -Wno-error=pessimizing-move -Wno-error=deprecated-declarations)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        target_compile_options(${target} PRIVATE /WX /bigobj /Zc:preprocessor)
    endif()
endforeach()

# Parser tests (issue #214) — only enabled when the parser is built.
# ParserTest.h is always in the source list (for IDE visibility) but
# its body is gated on NUMSIM_CAS_PARSER_ENABLED, defined here.
if(NUMSIM_CAS_BUILD_PARSER)
    target_compile_definitions(numsim_cas_test PRIVATE NUMSIM_CAS_PARSER_ENABLED)
    target_link_libraries(numsim_cas_test PRIVATE NumSim_CAS::Parser)
endif()

# Package install smoke test (PR #84): verifies that a downstream consumer
# can find_package(NumSim_CAS) from the install tree. Pure CMake test
# (no C++ compilation step). Gated on NUMSIM_CAS_ENABLE_PACKAGE_SMOKE_TEST
# because it requires a fully populated install prefix.
if(NUMSIM_CAS_ENABLE_PACKAGE_SMOKE_TEST)
  add_test(
    NAME numsim_cas_package_smoke
    COMMAND
      ${CMAKE_COMMAND}
      -DNUMSIM_CAS_SOURCE_DIR=${CMAKE_SOURCE_DIR}
      -DNUMSIM_CAS_BINARY_DIR=${CMAKE_BINARY_DIR}
      -DTEST_CONFIG=$<CONFIG>
      -P ${CMAKE_CURRENT_SOURCE_DIR}/package_smoke/package_smoke_test.cmake
  )
endif()
