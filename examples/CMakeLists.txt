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
add_numsim_cas_example(linear_elasticity linear_elasticity.cpp)

# Examples with programmatic asserts run as CTest tests so bit-rot is
# caught in CI. Add new examples here only if they exit non-zero on
# numerical mismatch.
enable_testing()
add_test(NAME example_linear_elasticity COMMAND linear_elasticity)
