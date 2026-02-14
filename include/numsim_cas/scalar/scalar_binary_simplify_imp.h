#ifndef SCALAR_BINARY_SIMPLIFY_IMP_H
#define SCALAR_BINARY_SIMPLIFY_IMP_H

#include <numsim_cas/scalar/simplifier/scalar_simplifier_add.h>
#include <numsim_cas/scalar/simplifier/scalar_simplifier_mul.h>
#include <numsim_cas/scalar/simplifier/scalar_simplifier_pow.h>
#include <numsim_cas/scalar/simplifier/scalar_simplifier_sub.h>

namespace numsim::cas {
// template <typename ExprTypeLHS, typename ExprTypeRHS>
// result_expression_t<ExprTypeLHS, ExprTypeRHS>
// binary_scalar_add_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
//   auto &_lhs{lhs.template get<scalar_visitable_t>()};
//   simplifier::add_base visitor(std::forward<ExprTypeLHS>(lhs),
//                                std::forward<ExprTypeRHS>(rhs));
//   return _lhs.accept(visitor);
// }

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// result_expression_t<ExprTypeLHS, ExprTypeRHS>
// binary_scalar_sub_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
//   auto &_lhs{lhs.template get<scalar_visitable_t>()};
//   simplifier::sub_base visitor(
//       std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
//   return _lhs.accept(visitor);
// }

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// result_expression_t<ExprTypeLHS, ExprTypeRHS>
// binary_scalar_div_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
//   auto &_lhs{lhs.template get<scalar_visitable_t>()};
//   simplifier::pow_base visitor(
//       std::forward<ExprTypeLHS>(lhs), -std::forward<ExprTypeRHS>(rhs));
//   return _lhs.accept(visitor);
// }

// template <typename ExprTypeLHS, typename ExprTypeRHS>
// result_expression_t<ExprTypeLHS, ExprTypeRHS>
// binary_scalar_mul_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
//   auto &_lhs{lhs.template get<scalar_visitable_t>()};
//   simplifier::mul_base visitor(
//       std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
//   return _lhs.accept(visitor);
// }

inline expression_holder<scalar_expression>
binary_scalar_pow_simplify(expression_holder<scalar_expression> lhs,
                           expression_holder<scalar_expression> rhs) {
  auto &_lhs{lhs.template get<scalar_visitable_t>()};
  simplifier::pow_base visitor(std::move(lhs), std::move(rhs));
  return _lhs.accept(visitor);
}
} // namespace numsim::cas

#endif // SCALAR_BINARY_SIMPLIFY_IMP_H
