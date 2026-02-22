#ifndef NUMSIM_CAS_H
#define NUMSIM_CAS_H

// scalar expression
#include <numsim_cas/scalar/scalar.h>
#include <numsim_cas/scalar/scalar_abs.h>
#include <numsim_cas/scalar/scalar_acos.h>
#include <numsim_cas/scalar/scalar_add.h>
#include <numsim_cas/scalar/scalar_asin.h>
#include <numsim_cas/scalar/scalar_atan.h>
#include <numsim_cas/scalar/scalar_constant.h>
#include <numsim_cas/scalar/scalar_cos.h>
#include <numsim_cas/scalar/scalar_exp.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_functions.h>
#include <numsim_cas/scalar/scalar_globals.h>
#include <numsim_cas/scalar/scalar_log.h>
#include <numsim_cas/scalar/scalar_mul.h>
#include <numsim_cas/scalar/scalar_named_expression.h>
#include <numsim_cas/scalar/scalar_negative.h>
#include <numsim_cas/scalar/scalar_one.h>
#include <numsim_cas/scalar/scalar_power.h>
#include <numsim_cas/scalar/scalar_sign.h>
#include <numsim_cas/scalar/scalar_sin.h>
#include <numsim_cas/scalar/scalar_sqrt.h>
#include <numsim_cas/scalar/scalar_tan.h>
#include <numsim_cas/scalar/scalar_visitor_typedef.h>
#include <numsim_cas/scalar/scalar_zero.h>

#include <numsim_cas/scalar/scalar_diff.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/scalar/visitors/scalar_differentiation.h>
#include <numsim_cas/scalar/visitors/scalar_evaluator.h>
#include <numsim_cas/scalar/visitors/scalar_printer.h>

// tensor expression
#include <numsim_cas/tensor/kronecker_delta.h>
#include <numsim_cas/tensor/tensor.h>
#include <numsim_cas/tensor/tensor_negative.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_zero.h>

#include <numsim_cas/tensor/tensor_assume.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_io.h>

// tensor based scalar expression
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_io.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

#endif // NUMSIM_CAS_H
