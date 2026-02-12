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
///
///
/// Enable constants in both cases only add one addtional node in variant
/// -- using a registry
/// using symbol_id = uint32_t;
/// struct constant_entry {
///   std::string name;               // for printing
///   long double approx_ld;          // optional numeric value for evaluation
///   // optional: units metadata, uncertainty, etc.
/// };
/// class constant_registry {
/// public:
///   symbol_id intern(std::string_view name, long double approx = 0.0L);
///   std::string_view name_of(symbol_id id) const { return entries[id].name; }
///   long double value_ld(symbol_id id) const { return entries[id].approx_ld; }
/// private:
///   std::vector<constant_entry> entries;                 // dense,
///   cache-friendly std::unordered_map<std::string, symbol_id> index;    //
///   only used by intern()
/// };
/// struct scalar_named_constant {
///   symbol_id id;   // stored in AST
/// };
///
/// -- using type-erasure and virtual functions
/// struct constant_concept {
/// virtual ~constant_concept() = default;
/// virtual std::string_view name() const = 0;
/// // For canonical ordering / hashing:
/// virtual uint64_t stable_key() const = 0;     // or compare(other)
/// virtual bool equals(const constant_concept&) const = 0;
/// // For numeric evaluation:
/// virtual long double value_ld() const = 0;    // or templated via visitor
/// };
/// struct scalar_constant_erased {
///   std::shared_ptr<const constant_concept> c;
/// };
///
///

// // generall
// #include <numsim_cas/expression.h>
// #include <numsim_cas/functions.h>
// #include <numsim_cas/numsim_cas_type_traits.h>
// #include <numsim_cas/utility_func.h>

// scalar expression
#include <numsim_cas/scalar/scalar.h>
#include <numsim_cas/scalar/scalar_abs.h>
#include <numsim_cas/scalar/scalar_acos.h>
#include <numsim_cas/scalar/scalar_add.h>
#include <numsim_cas/scalar/scalar_asin.h>
#include <numsim_cas/scalar/scalar_atan.h>
#include <numsim_cas/scalar/scalar_compare_less_visitor.h>
#include <numsim_cas/scalar/scalar_constant.h>
#include <numsim_cas/scalar/scalar_cos.h>
#include <numsim_cas/scalar/scalar_div.h>
#include <numsim_cas/scalar/scalar_exp.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_function.h>
#include <numsim_cas/scalar/scalar_functions.h>
#include <numsim_cas/scalar/scalar_globals.h>
#include <numsim_cas/scalar/scalar_log.h>
#include <numsim_cas/scalar/scalar_mul.h>
#include <numsim_cas/scalar/scalar_negativ.h>
#include <numsim_cas/scalar/scalar_one.h>
#include <numsim_cas/scalar/scalar_power.h>
#include <numsim_cas/scalar/scalar_sign.h>
#include <numsim_cas/scalar/scalar_sin.h>
#include <numsim_cas/scalar/scalar_sqrt.h>
#include <numsim_cas/scalar/scalar_tan.h>
#include <numsim_cas/scalar/scalar_visitor_typedef.h>
#include <numsim_cas/scalar/scalar_zero.h>

#include <numsim_cas/scalar/scalar_binary_simplify_imp.h>
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

#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_io.h>
// #include <numsim_cas/tensor/functions/basis_change.h>
// #include <numsim_cas/tensor/functions/inner_product_wrapper.h>
// #include <numsim_cas/tensor/functions/outer_product_wrapper.h>
// #include <numsim_cas/tensor/functions/simple_outer_product.h>
// #include <numsim_cas/tensor/functions/tensor_deviatoric.h>
// #include <numsim_cas/tensor/functions/tensor_inv.h>
// #include <numsim_cas/tensor/functions/tensor_pow.h>
// #include <numsim_cas/tensor/functions/tensor_power_diff.h>
// #include <numsim_cas/tensor/functions/tensor_symmetry.h>
// #include <numsim_cas/tensor/functions/tensor_volumetric.h>
// #include <numsim_cas/tensor/identity_tensor.h>
// #include <numsim_cas/tensor/operators/scalar/tensor_scalar_div.h>
// #include <numsim_cas/tensor/operators/scalar/tensor_scalar_mul.h>
// #include <numsim_cas/tensor/operators/tensor/tensor_add.h>
// #include <numsim_cas/tensor/operators/tensor/tensor_mul.h>
// #include <numsim_cas/tensor/operators/tensor/tensor_sub.h>
// #include
// >tensor/operators/tensor_to_scalar/tensor_to_scalar_with_tensor_div.h>
// #include
// >tensor/operators/tensor_to_scalar/tensor_to_scalar_with_tensor_mul.h>
// #include <numsim_cas/tensor/projection_tensor.h>
// #include <numsim_cas/tensor/scalar_tensor_op.h>

// #include <numsim_cas/tensor/tensor_compare_less_visitor.h>
// #include <numsim_cas/tensor/tensor_functions.h>
// #include <numsim_cas/tensor/tensor_globals.h>
// #include <numsim_cas/tensor/tensor_operators.h>
// #include <numsim_cas/tensor/tensor_std.h>
// #include <numsim_cas/tensor/tensor_zero.h>
// #include <numsim_cas/tensor/visitors/tensor_differentiation.h>
// #include <numsim_cas/tensor/visitors/tensor_printer.h>

// tensor based scalar expression
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_io.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>
// #include <numsim_cas/tensor_to_scalar/tensor_det.h>
// #include <numsim_cas/tensor_to_scalar/tensor_dot.h>
// #include <numsim_cas/tensor_to_scalar/tensor_inner_product_to_scalar.h>
// #include <numsim_cas/tensor_to_scalar/tensor_norm.h>
// #include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>
// #include <numsim_cas/tensor_to_scalar/tensor_to_scalar_log.h>
// #include <numsim_cas/tensor_to_scalar/tensor_to_scalar_negative.h>
// #include <numsim_cas/tensor_to_scalar/tensor_to_scalar_one.h>
// #include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>
// #include <numsim_cas/tensor_to_scalar/tensor_to_scalar_pow.h>
// #include <numsim_cas/tensor_to_scalar/tensor_to_scalar_scalar_wrapper.h>
// #include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>
// #include <numsim_cas/tensor_to_scalar/tensor_to_scalar_zero.h>
// #include <numsim_cas/tensor_to_scalar/tensor_trace.h>
// #include
// <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_differentiation.h>

// // generall headers
// #include <numsim_cas/operators.h>

// #include <numsim_cas/basic_functions.h>
// #include <numsim_cas/functions.h>
// #include <numsim_cas/scalar/scalar_operators.h>
// #include <numsim_cas/scalar/scalar_std.h>

// namespace std {
// template <typename... Args>
// struct hash<numsim::cas::expression_holder<Args...>> {
//   using type_t = numsim::cas::expression_holder<Args...>;
//   std::size_t operator()(const type_t &expr) const noexcept {
//     return expr.get().hash_value();
//   }
// };
// } // namespace std

// namespace numsim::cas {

// auto diff(expression_holder<tensor_expression> &expr,
//           expression_holder<tensor_expression> &arg) {
//   tensor_differentiation diff(arg);
//   return diff.apply(expr);
// }

// // template<typename ValueType>
// // auto diff(expression_holder<scalar_expression<ValueType>> & expr,
// // expression_holder<scalar_expression<ValueType>> & arg){
// //   scalar_differentiation<ValueType> diff(arg);
// //   return diff.apply(expr);
// // }

// std::unordered_map<expression_holder<scalar_expression>,
//                    std::vector<expression_holder<scalar_expression>>>
// group_by_common_factors(scalar_add &expr) {
//   using expr_type =
//       expression_holder<numsim::cas::scalar_expression>;
//   std::unordered_map<expr_type, std::vector<expr_type>> common_factors;

//   for (auto &term : expr) {
//     if (is_same<scalar_mul>(term.get())) {
//       // Look for factors inside multiplication terms
//       auto &mul_expr = term.template get<scalar_mul>();
//       for (auto &factor : mul_expr) {
//         // Group by the factor, e.g., if the factor is 'x', store all terms
//         // involving 'x'
//         common_factors[factor].push_back(term);
//       }
//     }
//   }

//   return common_factors;
// }

// } // Namespace numsim::cas
#endif // NUMSIM_CAS_H
