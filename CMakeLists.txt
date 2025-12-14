cmake_minimum_required(VERSION 3.22)

project(NumSim_CAS VERSION 1.0.0 LANGUAGES CXX DESCRIPTION "NumSim_CAS description")


set(CMAKE_CXX_STANDARD 23)
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
set(INSTALL_GTEST ON)
OPTION(NUMSIM_CAS_INSTALL_LIBRARY
       "Enable installing of tmech library into default locations"
       ${IS_TOPLEVEL_PROJECT})
OPTION(BUILD_EXAMPLES "NumSim_CAS examples" OFF)
OPTION(BUILD_TESTS "NumSim_CAS test suite" ON)
OPTION(BUILD_BENCHMARK "NumSim_CAS benchmark" OFF)
OPTION(DOWNLOAD_GTEST "build gtest from downloaded sources" OFF)
OPTION(DOWNLOAD_GBENCHMARK "download google benchmark and build from source" OFF)


find_package(tmech QUIET)
if(!TMECH_IS_DETECTED)
    FetchContent_Declare(
      tmech
      GIT_REPOSITORY https://github.com/petlenz/tmech.git
      GIT_TAG master
    )
    FetchContent_MakeAvailable(tmech)
endif()

set(PUBLIC_HEADER_FILES
    #common
    include/numsim_cas/numsim_cas.h
    include/numsim_cas/binary_op.h
    include/numsim_cas/expression.h
    include/numsim_cas/functions.h
    include/numsim_cas/n_ary_tree.h
    include/numsim_cas/n_ary_vector.h
    include/numsim_cas/symbol_base.h
    include/numsim_cas/numsim_cas_type_traits.h
    include/numsim_cas/unary_op.h
    include/numsim_cas/expression_holder.h
    include/numsim_cas/utility_func.h
    include/numsim_cas/nonlinear_solver_base.h
    include/numsim_cas/operators.h
    include/numsim_cas/get_hash_scalar.h
    include/numsim_cas/is_symbol.h
    include/numsim_cas/printer_base.h
    include/numsim_cas/numsim_cas_forward.h
    include/numsim_cas/expression_crtp.h
    include/numsim_cas/assumptions.h
    include/numsim_cas/basic_functions.h
    include/numsim_cas/simplify_rule_registry.h
    include/numsim_cas/numsim_cas_variant.h
    include/numsim_cas/update_hash.h
    include/numsim_cas/compare_less_visitor.h
    include/numsim_cas/compare_equal_visitor.h

    #tensor
    include/numsim_cas/tensor/tensor.h
    include/numsim_cas/tensor/functions/basis_change.h
    include/numsim_cas/tensor/functions/inner_product_wrapper.h
    include/numsim_cas/tensor/functions/outer_product_wrapper.h
    include/numsim_cas/tensor/functions/simple_outer_product.h
    include/numsim_cas/tensor/functions/tensor_symmetry.h
    include/numsim_cas/tensor/functions/tensor_deviatoric.h
    include/numsim_cas/tensor/functions/tensor_pow.h
    include/numsim_cas/tensor/functions/tensor_power_diff.h
    include/numsim_cas/tensor/functions/tensor_inv.h
    include/numsim_cas/tensor/data/tensor_data.h
    include/numsim_cas/tensor/data/tensor_data_add.h
    include/numsim_cas/tensor/data/tensor_data_basis_change.h
    include/numsim_cas/tensor/data/tensor_data_inner_product.h
    include/numsim_cas/tensor/data/tensor_data_outer_product.h
    include/numsim_cas/tensor/data/tensor_data_eval.h
    include/numsim_cas/tensor/data/tensor_data_sub.h
    include/numsim_cas/tensor/data/tensor_data_make_imp.h
    include/numsim_cas/tensor/operators/scalar/tensor_scalar_mul.h
    include/numsim_cas/tensor/operators/scalar/tensor_scalar_div.h
    include/numsim_cas/tensor/kronecker_delta.h
    include/numsim_cas/tensor/tensor_expression.h
    include/numsim_cas/tensor/tensor_negative.h
    include/numsim_cas/tensor/tensor_operators.h
    include/numsim_cas/tensor/tensor_functions.h
    include/numsim_cas/tensor/tensor_std.h
    include/numsim_cas/tensor/scalar_tensor_op.h
    include/numsim_cas/tensor/tensor_constant.h
    include/numsim_cas/tensor/tensor_zero.h
    include/numsim_cas/tensor/identity_tensor.h
    include/numsim_cas/tensor/operators/tensor/tensor_mul.h
    include/numsim_cas/tensor/operators/tensor/tensor_sub.h
    include/numsim_cas/tensor/operators/tensor/tensor_add.h
    include/numsim_cas/tensor/operators/tensor_to_scalar/tensor_to_scalar_with_tensor_div.h
    include/numsim_cas/tensor/operators/tensor_to_scalar/tensor_to_scalar_with_tensor_mul.h
    include/numsim_cas/tensor/simplifier/tensor_simplifier_add.h
    include/numsim_cas/tensor/simplifier/tensor_simplifier_sub.h
    include/numsim_cas/tensor/simplifier/tensor_simplifier_mul.h
    include/numsim_cas/tensor/simplifier/tensor_with_scalar_simplifier_mul.h
    include/numsim_cas/tensor/simplifier/tensor_with_scalar_simplifier_div.h
    include/numsim_cas/tensor/simplifier/tensor_with_tensor_to_scalar_simplifier_mul.h
    include/numsim_cas/tensor/simplifier/tensor_with_tensor_to_scalar_simplifier_div.h
    include/numsim_cas/tensor/simplifier/tensor_inner_product_simplifier.h
    include/numsim_cas/tensor/visitors/tensor_differentiation.h
    include/numsim_cas/tensor/visitors/tensor_printer.h
    include/numsim_cas/tensor/visitors/tensor_evaluator.h
    include/numsim_cas/tensor/tensor_functions_fwd.h
    include/numsim_cas/tensor/tensor_compare_less_visitor.h
    include/numsim_cas/tensor/tensor_globals.h
    include/numsim_cas/tensor/projection_tensor.h

    #scalar
    include/numsim_cas/scalar/scalar.h
    include/numsim_cas/scalar/scalar_expression.h
    include/numsim_cas/scalar/scalar_sin.h
    include/numsim_cas/scalar/scalar_cos.h
    include/numsim_cas/scalar/scalar_tan.h
    include/numsim_cas/scalar/scalar_atan.h
    include/numsim_cas/scalar/scalar_asin.h
    include/numsim_cas/scalar/scalar_acos.h
    include/numsim_cas/scalar/scalar_one.h
    include/numsim_cas/scalar/scalar_zero.h
    include/numsim_cas/scalar/scalar_exp.h
    include/numsim_cas/scalar/scalar_sqrt.h
    include/numsim_cas/scalar/scalar_log.h
    include/numsim_cas/scalar/scalar_power.h
    include/numsim_cas/scalar/scalar_abs.h
    include/numsim_cas/scalar/scalar_sign.h
    include/numsim_cas/scalar/scalar_std.h
    include/numsim_cas/scalar/scalar_globals.h
    include/numsim_cas/scalar/simplifier/scalar_simplifier_add.h
    include/numsim_cas/scalar/simplifier/scalar_simplifier_mul.h
    include/numsim_cas/scalar/simplifier/scalar_simplifier_sub.h
    include/numsim_cas/scalar/simplifier/scalar_simplifier_negative.h
    include/numsim_cas/scalar/simplifier/scalar_simplifier_div.h
    include/numsim_cas/scalar/scalar_functions.h
    include/numsim_cas/scalar/scalar_constant.h
    include/numsim_cas/scalar/scalar_add.h
    include/numsim_cas/scalar/scalar_negativ.h
    include/numsim_cas/scalar/scalar_div.h
    include/numsim_cas/scalar/scalar_mul.h
    include/numsim_cas/scalar/scalar_operators.h
    include/numsim_cas/scalar/scalar_visitor_typedef.h
    include/numsim_cas/scalar/visitors/scalar_differentiation.h
    include/numsim_cas/scalar/visitors/scalar_printer.h
    include/numsim_cas/scalar/visitors/scalar_evaluator.h
    include/numsim_cas/scalar/scalar_function.h
    include/numsim_cas/scalar/scalar_functions_fwd.h
    include/numsim_cas/scalar/scalar_compare_less_visitor.h

    #tensor valued scalar function
    include/numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h
    include/numsim_cas/tensor_to_scalar/tensor_to_scalar_negative.h
    include/numsim_cas/tensor_to_scalar/tensor_trace.h
    include/numsim_cas/tensor_to_scalar/tensor_det.h
    include/numsim_cas/tensor_to_scalar/tensor_norm.h
    include/numsim_cas/tensor_to_scalar/tensor_dot.h
    include/numsim_cas/tensor_to_scalar/tensor_to_scalar_pow.h
    include/numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h
    include/numsim_cas/tensor_to_scalar/tensor_inner_product_to_scalar.h
    include/numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h
    include/numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_add.h
    include/numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_mul.h
    include/numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_sub.h
    include/numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_div.h
    include/numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_with_scalar_simplifier_add.h
    include/numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_with_scalar_simplifier_mul.h
    include/numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_with_scalar_simplifier_div.h
    include/numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_with_scalar_simplifier_sub.h
    include/numsim_cas/tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_mul.h
    include/numsim_cas/tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_add.h
    include/numsim_cas/tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_sub.h
    include/numsim_cas/tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_div.h
    include/numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_printer.h
    include/numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_differentiation.h
    include/numsim_cas/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_sub.h
    include/numsim_cas/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_mul.h
    include/numsim_cas/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_add.h
    include/numsim_cas/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_div.h
    include/numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h
    include/numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h
    include/numsim_cas/tensor_to_scalar/tensor_to_scalar_functions_fwd.h
    include/numsim_cas/tensor_to_scalar/tensor_to_scalar_one.h
    include/numsim_cas/tensor_to_scalar/tensor_to_scalar_zero.h
)

add_library(${PROJECT_NAME} INTERFACE
    ${PUBLIC_HEADER_FILES}
)

target_compile_definitions(${PROJECT_NAME} INTERFACE NUMSIM_CAS_LIBRARY)
target_link_libraries(${PROJECT_NAME} INTERFACE tmech::tmech)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>  # <prefix>/include/mylib
)

include(GNUInstallDirs)

if(NUMSIM_CAS_INSTALL_LIBRARY)
    install(
        TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )
    install(FILES ${PUBLIC_HEADER_FILES} DESTINATION include/numsim_cas)
endif()

install(TARGETS NumSim_CAS
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


if(BUILD_TESTS)
    enable_testing()
    include(CTest)
    add_subdirectory(tests)
endif()
