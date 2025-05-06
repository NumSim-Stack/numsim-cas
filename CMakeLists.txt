cmake_minimum_required(VERSION 3.14)

project(symTMVar VERSION 1.0.0 LANGUAGES CXX DESCRIPTION "symTM description")


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

 if (MSVC)
   message("currently MSVC compiler" ${MSVC_TOOLSET_VERSION})
   
    if(${MSVC_TOOLSET_VERSION} LESS 142)
        set(TMECH_IS_DETECTED ON)
    endif()
endif()

set(CMAKE_BUILD_TYPE Debug)

include(FetchContent)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(IS_TOPLEVEL_PROJECT TRUE)
else()
  set(IS_TOPLEVEL_PROJECT FALSE)
endif()


# do not install google test framework
set(INSTALL_GTEST OFF)
OPTION(SYMTM_INSTALL_LIBRARY
       "Enable installing of tmech library into default locations"
       ${IS_TOPLEVEL_PROJECT})
OPTION(BUILD_EXAMPLES "symTM examples" OFF)
OPTION(BUILD_TESTS "symTM test suite" ON)
OPTION(BUILD_BENCHMARK "symTM benchmark" OFF)
OPTION(DOWNLOAD_GTEST "build gtest from downloaded sources" OFF)
OPTION(DOWNLOAD_GBENCHMARK "download google benchmark and build from source" OFF)

#FetchContent_Declare(
#  tmech
#  GIT_REPOSITORY https://github.com/petlenz/tmech.git
#  GIT_TAG master
#)
#FetchContent_MakeAvailable(tmech)

set(INCLUDE_PATH include/symTM)
set(INCLUDE_PATH_COMMON ${INCLUDE_PATH}/common)
set(INCLUDE_PATH_SCALAR ${INCLUDE_PATH}/scalar)
set(INCLUDE_PATH_TENSOR ${INCLUDE_PATH}/tensor)

set(SRC_PATH src)

set(PUBLIC_HEADER_FILES
    #common
    ${INCLUDE_PATH}/symTM.h
    ${INCLUDE_PATH}/binary_op.h
    ${INCLUDE_PATH}/expression.h
    ${INCLUDE_PATH}/functions.h
    ${INCLUDE_PATH}/n_ary_tree.h
    ${INCLUDE_PATH}/symbol_base.h
    ${INCLUDE_PATH}/symTM_type_traits.h
    ${INCLUDE_PATH}/unary_op.h
    ${INCLUDE_PATH}/expression_holder.h
    include/symTM/utility_func.h
    include/symTM/nonlinear_solver_base.h
    include/symTM/operators.h
    include/symTM/get_hash_scalar.h
    include/symTM/is_symbol.h
    include/symTM/printer_base.h
    include/symTM/symTM_forward.h
    include/symTM/expression_crtp.h
    include/symTM/assumptions.h
    include/symTM/basic_functions.h
    include/symTM/simplify_rule_registry.h
    include/symTM/symTM_variant.h

    #tensor
    ${INCLUDE_PATH_TENSOR}/basis_change.h
    ${INCLUDE_PATH_TENSOR}/inner_product_wrapper.h
    ${INCLUDE_PATH_TENSOR}/outer_product_wrapper.h
    ${INCLUDE_PATH_TENSOR}/tensor.h
    ${INCLUDE_PATH_TENSOR}/tensor_add.h
    ${INCLUDE_PATH_TENSOR}/tensor_data.h
    ${INCLUDE_PATH_TENSOR}/tensor_data_add.h
    ${INCLUDE_PATH_TENSOR}/tensor_data_basis_change.h
    ${INCLUDE_PATH_TENSOR}/tensor_data_inner_product.h
    ${INCLUDE_PATH_TENSOR}/tensor_data_outer_product.h
    ${INCLUDE_PATH_TENSOR}/tensor_sub.h
    ${INCLUDE_PATH_TENSOR}/tensor_evaluator.h
    ${INCLUDE_PATH_TENSOR}/tensor_scalar_mul.h
    ${INCLUDE_PATH_TENSOR}/tensor_scalar_div.h
    ${INCLUDE_PATH_TENSOR}/kronecker_delta.h
    ${INCLUDE_PATH_TENSOR}/simple_outer_product.h
    ${INCLUDE_PATH_TENSOR}/tensor_expression.h
    ${INCLUDE_PATH_TENSOR}/tensor_scalar_expression.h
    ${INCLUDE_PATH_TENSOR}/tensor_negative.h
    ${INCLUDE_PATH_TENSOR}/make_tensor_data_imp.h
    ${INCLUDE_PATH_TENSOR}/tensor_data_eval.h
    ${INCLUDE_PATH_TENSOR}/tensor_data_sub.h
    ${INCLUDE_PATH_TENSOR}/tensor_differentiation.h
    ${INCLUDE_PATH_TENSOR}/tensor_printer.h
    ${INCLUDE_PATH_TENSOR}/tensor_operators.h
    ${INCLUDE_PATH_TENSOR}/tensor_functions.h
    ${INCLUDE_PATH_TENSOR}/tensor_symmetry.h
    ${INCLUDE_PATH_TENSOR}/tensor_std.h
    ${INCLUDE_PATH_TENSOR}/scalar_tensor_op.h
    ${INCLUDE_PATH_TENSOR}/tensor_visitor_typedefs.h
    ${INCLUDE_PATH_TENSOR}/tensor_constant.h
    ${INCLUDE_PATH_TENSOR}/tensor_zero.h
    ${INCLUDE_PATH_TENSOR}/simplifier/tensor_simplifier_add.h
    ${INCLUDE_PATH_TENSOR}/simplifier/tensor_simplifier_sub.h
    include/symTM/tensor/simplifier/tensor_simplifier_mul.h
    include/symTM/tensor/simplifier/tensor_with_scalar_simplifier_mul.h
    include/symTM/tensor/simplifier/tensor_with_scalar_simplifier_div.h

    #scalar
    ${INCLUDE_PATH_SCALAR}/scalar.h
    ${INCLUDE_PATH_SCALAR}/scalar_expression.h
    ${INCLUDE_PATH_SCALAR}/scalar_sin.h
    ${INCLUDE_PATH_SCALAR}/scalar_cos.h
    ${INCLUDE_PATH_SCALAR}/scalar_tan.h
    ${INCLUDE_PATH_SCALAR}/scalar_atan.h
    ${INCLUDE_PATH_SCALAR}/scalar_asin.h
    ${INCLUDE_PATH_SCALAR}/scalar_acos.h
    ${INCLUDE_PATH_SCALAR}/scalar_one.h
    ${INCLUDE_PATH_SCALAR}/scalar_zero.h
    ${INCLUDE_PATH_SCALAR}/scalar_exp.h
    ${INCLUDE_PATH_SCALAR}/scalar_sqrt.h
    ${INCLUDE_PATH_SCALAR}/scalar_log.h
    ${INCLUDE_PATH_SCALAR}/scalar_power.h
    ${INCLUDE_PATH_SCALAR}/scalar_abs.h
    ${INCLUDE_PATH_SCALAR}/scalar_sign.h
    ${INCLUDE_PATH_SCALAR}/scalar_sub.h
    ${INCLUDE_PATH_SCALAR}/scalar_std.h
    ${INCLUDE_PATH_SCALAR}/scalar_globals.h
    ${INCLUDE_PATH_SCALAR}/simplifier/scalar_simplifier_add.h
    ${INCLUDE_PATH_SCALAR}/simplifier/scalar_simplifier_mul.h
    ${INCLUDE_PATH_SCALAR}/simplifier/scalar_simplifier_sub.h
    include/symTM/scalar/simplifier/scalar_simplifier_div.h
    ${INCLUDE_PATH_SCALAR}/scalar_functions.h
    ${INCLUDE_PATH_SCALAR}/scalar_constant.h
    ${INCLUDE_PATH_SCALAR}/scalar_add.h
    ${INCLUDE_PATH_SCALAR}/scalar_negativ.h
    ${INCLUDE_PATH_SCALAR}/scalar_div.h
    ${INCLUDE_PATH_SCALAR}/scalar_mul.h
    ${INCLUDE_PATH_SCALAR}/scalar_operators.h
    ${INCLUDE_PATH_SCALAR}/scalar_visitor_typedef.h
    ${INCLUDE_PATH_SCALAR}/visitors/scalar_differentiation.h
    ${INCLUDE_PATH_SCALAR}/visitors/scalar_printer.h
    ${INCLUDE_PATH_SCALAR}/visitors/scalar_evaluator.h

    #tensor valued scalar function
    include/symTM/tensor_to_scalar/tensor_to_scalar_expression.h
    include/symTM/tensor_to_scalar/tensor_to_scalar_negative.h
    include/symTM/tensor_to_scalar/tensor_trace.h
    include/symTM/tensor_to_scalar/tensor_det.h
    include/symTM/tensor_to_scalar/tensor_norm.h
    include/symTM/tensor_to_scalar/tensor_dot.h
    include/symTM/tensor_to_scalar/tensor_to_scalar_pow.h
    include/symTM/tensor_to_scalar/tensor_to_scalar_operators.h
    include/symTM/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_add.h
    include/symTM/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_mul.h
    include/symTM/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_sub.h
    include/symTM/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_div.h
    include/symTM/tensor_to_scalar/simplifier/tensor_to_scalar_with_scalar_simplifier_add.h
    include/symTM/tensor_to_scalar/simplifier/tensor_to_scalar_with_scalar_simplifier_mul.h
    include/symTM/tensor_to_scalar/simplifier/tensor_to_scalar_with_scalar_simplifier_div.h
    include/symTM/tensor_to_scalar/simplifier/tensor_to_scalar_with_scalar_simplifier_sub.h
    include/symTM/tensor_to_scalar/simplifier/tensor_to_scalar_with_tensor_simplifier_mul.h
    include/symTM/tensor_to_scalar/simplifier/tensor_to_scalar_with_tensor_simplifier_div.h
    include/symTM/tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_mul.h
    include/symTM/tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_add.h
    include/symTM/tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_sub.h
    include/symTM/tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_div.h
    include/symTM/tensor_to_scalar/operators/tensor_to_scalar_with_tensor/tensor_to_scalar_with_tensor_mul.h
    include/symTM/tensor_to_scalar/operators/tensor_to_scalar_with_tensor/tensor_to_scalar_with_tensor_div.h
    include/symTM/tensor_to_scalar/visitors/tensor_scalar_printer.h
    include/symTM/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_sub.h
    include/symTM/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_mul.h
    include/symTM/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_add.h
    include/symTM/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_div.h
)

add_library(${PROJECT_NAME} INTERFACE
    ${PUBLIC_HEADER_FILES}
)

target_compile_definitions(${PROJECT_NAME} INTERFACE SYMTM_LIBRARY)
#target_link_libraries(symTM tmech::tmech)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/symTM>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>  # <prefix>/include/mylib
)

include(GNUInstallDirs)

if(SYMTM_INSTALL_LIBRARY)
    install(
        TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )
    install(FILES ${PUBLIC_HEADER_FILES} DESTINATION include/symTM)
endif()

#install(TARGETS symTM
#    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


if(BUILD_TESTS)
    enable_testing()
    include(CTest)
    add_subdirectory(tests)
endif()
