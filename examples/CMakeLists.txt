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
