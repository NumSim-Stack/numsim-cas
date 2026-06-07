cmake_minimum_required(VERSION 3.22)
project(numsim_cas_examples)
set(CMAKE_CXX_STANDARD 23)

macro(add_numsim_cas_example TARGET_NAME)
    add_executable(${TARGET_NAME} ${ARGN})
    target_link_libraries(${TARGET_NAME} PRIVATE NumSim_CAS)
endmacro()

add_numsim_cas_example(scalar_basics scalar_basics.cpp)
add_numsim_cas_example(scalar_differentiation scalar_differentiation.cpp)
add_numsim_cas_example(scalar_evaluation scalar_evaluation.cpp)
add_numsim_cas_example(tensor_basics tensor_basics.cpp)
add_numsim_cas_example(tensor_to_scalar_basics tensor_to_scalar_basics.cpp)
add_numsim_cas_example(assumptions_basics assumptions_basics.cpp)
add_numsim_cas_example(linear_elasticity linear_elasticity.cpp)

# Parser examples — only built when NUMSIM_CAS_BUILD_PARSER=ON, since
# they link against the parser library target. Beyond demonstrating
# the public API, these examples act as an include-hygiene check: if
# any of the public parser headers ever started transitively
# including PEGTL, that leak would surface here as new symbols in
# downstream TUs that don't otherwise touch PEGTL.
macro(add_numsim_cas_parser_example TARGET_NAME)
    add_executable(${TARGET_NAME} ${ARGN})
    target_link_libraries(${TARGET_NAME} PRIVATE NumSim_CAS NumSim_CAS::Parser)
endmacro()

if(NUMSIM_CAS_BUILD_PARSER)
    add_numsim_cas_parser_example(parser_basics parser_basics.cpp)
    add_numsim_cas_parser_example(parser_symbol_table parser_symbol_table.cpp)
    add_numsim_cas_parser_example(parser_error_handling parser_error_handling.cpp)
    add_numsim_cas_parser_example(parser_tensor_diff parser_tensor_diff.cpp)
endif()

# Examples with programmatic asserts run as CTest tests so bit-rot is
# caught in CI. Add new examples here only if they exit non-zero on
# numerical mismatch.
enable_testing()
add_test(NAME example_linear_elasticity COMMAND linear_elasticity)
add_test(NAME example_assumptions_basics COMMAND assumptions_basics)
