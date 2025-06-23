#ifndef NUMSIM_CAS_H
#define NUMSIM_CAS_H

/// TODO
/// General
///   basis_change.h   --> basis_change_wrapper.h
///   basis_change_imp --> basis_change_wrapper
///   visitor concept for not same arguemts

/// TODO
/// return Expression<ExprType> ExprType := {scalar_expression,
/// tensor_expression, tensor_scalar_expression} operator(Expression<ExprType>
/// &&, Expression<ExprType> &&) template based counter Expression types
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

// generall
#include "expression.h"
#include "functions.h"
#include "numsim_cas_type_traits.h"
#include "utility_func.h"

// tensor expression
#include "tensor/functions/basis_change.h"
#include "tensor/functions/inner_product_wrapper.h"
#include "tensor/functions/outer_product_wrapper.h"
#include "tensor/functions/simple_outer_product.h"
#include "tensor/functions/tensor_pow.h"
#include "tensor/functions/tensor_symmetry.h"
#include "tensor/operators/scalar/tensor_scalar_div.h"
#include "tensor/operators/scalar/tensor_scalar_mul.h"
#include "tensor/operators/tensor/tensor_add.h"
#include "tensor/operators/tensor/tensor_mul.h"
#include "tensor/operators/tensor/tensor_sub.h"
#include "tensor/scalar_tensor_op.h"
#include "tensor/tensor.h"
#include "tensor/tensor_functions.h"
#include "tensor/tensor_operators.h"
#include "tensor/tensor_std.h"
#include "tensor/tensor_zero.h"
#include "tensor/visitors/tensor_differentiation.h"
#include "tensor/visitors/tensor_printer.h"

// tensor based scalar expression
#include "tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_add.h"
#include "tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_div.h"
#include "tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_mul.h"
#include "tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_sub.h"
#include "tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_add.h"
#include "tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_div.h"
#include "tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_mul.h"
#include "tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_sub.h"
#include "tensor_to_scalar/tensor_det.h"
#include "tensor_to_scalar/tensor_dot.h"
#include "tensor_to_scalar/tensor_inner_product_to_scalar.h"
#include "tensor_to_scalar/tensor_norm.h"
#include "tensor_to_scalar/tensor_to_scalar_expression.h"
#include "tensor_to_scalar/tensor_to_scalar_negative.h"
#include "tensor_to_scalar/tensor_to_scalar_operators.h"
#include "tensor_to_scalar/tensor_to_scalar_pow.h"
#include "tensor_to_scalar/tensor_trace.h"

// scalar expression
#include "scalar/scalar.h"
#include "scalar/scalar_abs.h"
#include "scalar/scalar_acos.h"
#include "scalar/scalar_add.h"
#include "scalar/scalar_asin.h"
#include "scalar/scalar_atan.h"
#include "scalar/scalar_constant.h"
#include "scalar/scalar_cos.h"
#include "scalar/scalar_div.h"
#include "scalar/scalar_exp.h"
#include "scalar/scalar_globals.h"
#include "scalar/scalar_log.h"
#include "scalar/scalar_mul.h"
#include "scalar/scalar_negativ.h"
#include "scalar/scalar_one.h"
#include "scalar/scalar_power.h"
#include "scalar/scalar_sign.h"
#include "scalar/scalar_sin.h"
#include "scalar/scalar_sqrt.h"
#include "scalar/scalar_std.h"
#include "scalar/scalar_tan.h"
#include "scalar/scalar_visitor_typedef.h"
#include "scalar/scalar_zero.h"

#include "scalar/scalar_operators.h"
#include "scalar/visitors/scalar_differentiation.h"
#include "scalar/visitors/scalar_evaluator.h"
#include "scalar/visitors/scalar_printer.h"

// generall headers
#include "operators.h"

#include "basic_functions.h"
#include "functions.h"
#include "scalar/scalar_operators.h"
#include "scalar/scalar_std.h"

namespace std {
template <typename... Args>
struct hash<numsim::cas::expression_holder<Args...>> {
  using type_t = numsim::cas::expression_holder<Args...>;
  std::size_t operator()(const type_t &expr) const noexcept {
    return expr.get().hash_value();
  }
};
} // namespace std

namespace numsim::cas {

template <typename ValueType>
auto diff(expression_holder<tensor_expression<ValueType>> &expr,
          expression_holder<tensor_expression<ValueType>> &arg) {
  tensor_differentiation<ValueType> diff(arg);
  return diff.apply(expr);
}

// template<typename ValueType>
// auto diff(expression_holder<scalar_expression<ValueType>> & expr,
// expression_holder<scalar_expression<ValueType>> & arg){
//   scalar_differentiation<ValueType> diff(arg);
//   return diff.apply(expr);
// }

template <typename ValueType>
std::unordered_map<expression_holder<scalar_expression<ValueType>>,
                   std::vector<expression_holder<scalar_expression<ValueType>>>>
group_by_common_factors(scalar_add<ValueType> &expr) {
  using expr_type =
      expression_holder<numsim::cas::scalar_expression<ValueType>>;
  std::unordered_map<expr_type, std::vector<expr_type>> common_factors;

  for (auto &term : expr) {
    if (is_same<scalar_mul<ValueType>>(term.get())) {
      // Look for factors inside multiplication terms
      auto &mul_expr = term.template get<scalar_mul<ValueType>>();
      for (auto &factor : mul_expr) {
        // Group by the factor, e.g., if the factor is 'x', store all terms
        // involving 'x'
        common_factors[factor].push_back(term);
      }
    }
  }

  return common_factors;
}

} // Namespace numsim::cas
#endif // NUMSIM_CAS_H
