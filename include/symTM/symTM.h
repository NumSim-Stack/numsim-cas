#ifndef SYMTMECH_H
#define SYMTMECH_H

/// TODO
/// General
///   basis_change.h   --> basis_change_wrapper.h
///   basis_change_imp --> basis_change_wrapper
///   visitor concept for not same arguemts
///
///
/// Scalar expression
///
/// Tensor-Scalar expression
///
///
/// TODO
/// pattern matching based on templates
/// use new approach for scalar_simplifier_mul
/// cleanup scalar stuff
/// cleanup tensor stuff
/// cleanup common stuff
/// cleanup cmake script (include dir and so on)
/// generate doxygen code
/// merge two n_ary_trees
///   --> use std::function for special handling?
///   --> add (x+y+z) + (x+a) --> 2*x+y+z+a --> mul
///   --> mul (x*y*z) * (x*a) --> pow(x,2)*y*z*a --> pow
/// tensor_to_scalar::implifier::mul_base x*pow(x,constant) --> pow(x,constant+1)

/// TODO
/// return Expression<ExprType> ExprType := {scalar_expression, tensor_expression, tensor_scalar_expression}
/// operator(Expression<ExprType> &&, Expression<ExprType> &&)
/// template based counter
/// Expression types
///  * scalar valued functions S : R -> R
///  * tensor valued functions T : R^{k} -> R^{k}
///  * tensor valued functions R : R^{k} -> R
///  * complex
///
/// What about mixed expression types?
/// SxT -> T
/// SxR -> R
/// RxT -> T
///
///
/// expression_holder_imp
/// - operator-()
/// - tensor_expression
/// - scalar_expression
/// - tensor_scalar_expression
///
///
/// tensor_expression / scalar_expression / tensor_scalar_expression
/// why not in symbol_base? what about m_expr in symbol_base?
/// how to handle that in a symbol_base based class
/// - operator-()
/// - operator+=()
/// - operator-=()
///
/// simplifyer
/// - basis_change<1,2,3,4,...,R> --> remove
///
/// make base class contructor protected

/// ast_transformer_base
/// apply(expression)
/// --> ptr expression
///
/// ast_printer_base
/// apply(expression)
/// --> void
/// Tensor expression
/// * simple_outer_product as an n_ary_tree, e.g. ((a otimes b) otimes c)


//generall
#include "symTM_type_traits.h"
#include "expression.h"
#include "functions.h"
#include "utility_func.h"

// tensor expression
#include "tensor/tensor.h"
#include "tensor/tensor_sub.h"
#include "tensor/tensor_add.h"
#include "tensor/inner_product_wrapper.h"
#include "tensor/basis_change.h"
#include "tensor/outer_product_wrapper.h"
#include "tensor/scalar_tensor_op.h"
#include "tensor/simple_outer_product.h"
#include "tensor/tensor_operators.h"
#include "tensor/tensor_differentiation.h"
#include "tensor/tensor_printer.h"
#include "tensor/tensor_symmetry.h"
#include "tensor/tensor_std.h"
#include "tensor/tensor_differentiation.h"
#include "tensor/tensor_zero.h"
#include "tensor/tensor_scalar_mul.h"
#include "tensor/tensor_scalar_div.h"

// tensor based scalar expression
#include "tensor_to_scalar/tensor_to_scalar_expression.h"
#include "tensor_to_scalar/tensor_to_scalar_operators.h"
#include "tensor_to_scalar/tensor_to_scalar_negative.h"
#include "tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_add.h"
#include "tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_mul.h"
#include "tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_sub.h"
#include "tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_div.h"
#include "tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_add.h"
#include "tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_mul.h"
#include "tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_sub.h"
#include "tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_div.h"
#include "tensor_to_scalar/tensor_trace.h"
#include "tensor_to_scalar/tensor_det.h"
#include "tensor_to_scalar/tensor_norm.h"
#include "tensor_to_scalar/tensor_dot.h"
#include "tensor_to_scalar/tensor_to_scalar_pow.h"

// scalar expression
#include "symTM/scalar/scalar.h"
#include "symTM/scalar/scalar_one.h"
#include "symTM/scalar/scalar_zero.h"
#include "symTM/scalar/scalar_constant.h"
#include "symTM/scalar/scalar_add.h"
#include "symTM/scalar/scalar_div.h"
#include "symTM/scalar/scalar_mul.h"
#include "symTM/scalar/scalar_negativ.h"
#include "symTM/scalar/scalar_sin.h"
#include "symTM/scalar/scalar_cos.h"
#include "symTM/scalar/scalar_tan.h"
#include "symTM/scalar/scalar_asin.h"
#include "symTM/scalar/scalar_acos.h"
#include "symTM/scalar/scalar_atan.h"
#include "symTM/scalar/scalar_exp.h"
#include "symTM/scalar/scalar_power.h"
#include "symTM/scalar/scalar_abs.h"
#include "symTM/scalar/scalar_sign.h"
#include "symTM/scalar/scalar_sqrt.h"
#include "symTM/scalar/scalar_log.h"
#include "scalar/scalar_std.h"

#include "symTM/scalar/scalar_visitor_typedef.h"

#include "symTM/scalar/scalar_operators.h"
#include "symTM/scalar/visitors/scalar_evaluator.h"
#include "symTM/scalar/visitors/scalar_differentiation.h"
#include "symTM/scalar/visitors/scalar_printer.h"

// generall headers
#include "operators.h"

#include "symTM/scalar/scalar_operators.h"
#include "symTM/scalar/scalar_std.h"
#include "functions.h"
#include "basic_functions.h"


namespace symTM {

template<typename ValueType>
auto diff(expression_holder<tensor_expression<ValueType>> & expr, expression_holder<tensor_expression<ValueType>> & arg){
  tensor_differentiation<ValueType> diff(arg);
  return diff.apply(expr);
}

//template<typename ValueType>
//auto diff(expression_holder<scalar_expression<ValueType>> & expr, expression_holder<scalar_expression<ValueType>> & arg){
//  scalar_differentiation<ValueType> diff(arg);
//  return diff.apply(expr);
//}



template<typename ValueType>
std::unordered_map<expression_holder<scalar_expression<ValueType>>, std::vector<expression_holder<scalar_expression<ValueType>>>>
group_by_common_factors(scalar_add<ValueType>& expr) {
  using expr_type = expression_holder<symTM::scalar_expression<ValueType>>;
  std::unordered_map<expr_type, std::vector<expr_type>> common_factors;

  for (auto& term : expr) {
    if (is_same<scalar_mul<ValueType>>(term.get())) {
      // Look for factors inside multiplication terms
      auto& mul_expr = term.template get<scalar_mul<ValueType>>();
      for (auto& factor : mul_expr) {
        // Group by the factor, e.g., if the factor is 'x', store all terms involving 'x'
        common_factors[factor].push_back(term);
      }
    }
  }

  return common_factors;
}

}
#endif // SYMTMECH_H
