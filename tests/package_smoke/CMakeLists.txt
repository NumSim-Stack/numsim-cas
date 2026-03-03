cmake_minimum_required(VERSION 3.22)

project(numsim_cas_package_smoke LANGUAGES CXX)

find_package(NumSim_CAS CONFIG REQUIRED)

add_executable(numsim_cas_package_smoke main.cpp)
target_link_libraries(numsim_cas_package_smoke PRIVATE NumSim_CAS::NumSim_CAS)
target_compile_features(numsim_cas_package_smoke PRIVATE cxx_std_23)
