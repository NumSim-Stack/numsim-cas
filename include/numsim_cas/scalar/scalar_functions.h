#ifndef SCALAR_FUNCTIONS_H
#define SCALAR_FUNCTIONS_H

#include "../expression_holder.h"
#include "../numsim_cas_type_traits.h"
#include "../unary_op.h"
#include "scalar_expression.h"
#include "simplifier/scalar_simplifier_add.h"
#include "simplifier/scalar_simplifier_div.h"
#include "simplifier/scalar_simplifier_mul.h"
#include "simplifier/scalar_simplifier_sub.h"
#include <ranges>

namespace numsim::cas {

template <typename ExprTypeLHS, typename ExprTypeRHS>
constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_scalar_add_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
  //  auto &_lhs = *lhs;
  //  auto &_rhs = *rhs;
  //  return std::visit(
  //      scalar_simplifier_add<ExprTypeLHS,ExprTypeRHS>(std::forward<ExprTypeLHS>(lhs),
  //      std::forward<ExprTypeRHS>(rhs)), _lhs, _rhs);
  return visit(
      simplifier::add_base<ExprTypeLHS, ExprTypeRHS>(
          std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
      *lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_scalar_sub_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
  //  auto &_lhs = *lhs;
  //  auto &_rhs = *rhs;
  //  return std::visit(
  //      scalar_simplifier_add<ExprTypeLHS,ExprTypeRHS>(std::forward<ExprTypeLHS>(lhs),
  //      std::forward<ExprTypeRHS>(rhs)), _lhs, _rhs);
  return visit(
      simplifier::sub_base<ExprTypeLHS, ExprTypeRHS>(
          std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
      *lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_scalar_div_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
  //  using value_type = typename ExprTypeLHS::value_type;
  //  return make_expression<scalar_div<value_type>>(
  //      std::forward<ExprTypeRHS>(lhs), std::forward<ExprTypeLHS>(rhs));
  auto &_lhs = *lhs;
  return visit(
      scalar_detail::simplifier::div_base<ExprTypeLHS, ExprTypeRHS>(
          std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
      _lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
constexpr inline result_expression_t<ExprTypeLHS, ExprTypeRHS>
binary_scalar_mul_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
  //  auto &_lhs = *lhs;
  //  auto &_rhs = *rhs;
  //  return std::visit(
  //      scalar_simplifier_mul<ExprTypeLHS,ExprTypeRHS>(std::forward<ExprTypeLHS>(lhs),
  //      std::forward<ExprTypeRHS>(rhs)), _lhs, _rhs);
  return visit(
      simplifier::mul_base<ExprTypeLHS, ExprTypeRHS>(
          std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
      *lhs);
}

namespace detail {

template <typename ValueType> class contains_symbol {
public:
  contains_symbol() {}

  template <typename T> constexpr inline bool operator()(T const &) {
    return false;
  }

  constexpr inline bool
  operator()([[maybe_unused]] scalar<ValueType> const &visitable) {
    return true;
  }

  template <typename Derived>
  constexpr inline bool
  operator()(unary_op<Derived, scalar_expression<ValueType>> const &visitable) {
    return std::visit(*this, visitable.expr().get());
  }

  //  template <typename Derived>
  //  constexpr inline bool operator()(
  //      binary_op<Derived, scalar_expression<ValueType>> const &visitable) {
  //    return std::visit(*this, visitable.expr_lhs().get()) ||
  //           std::visit(*this, visitable.expr_rhs().get());
  //  }

  template <typename Derived>
  constexpr inline bool operator()(
      n_ary_tree<scalar_expression<ValueType>, Derived> const &visitable) {
    for (auto &child : visitable.hash_map() | std::views::values) {
      if (std::visit(*this, child.get())) {
        return true;
      }
    }
    return false;
  }
};
} // namespace detail

template <typename ValueType>
bool contains_symbol(
    expression_holder<scalar_expression<ValueType>> const &expr) {
  return std::visit(detail::contains_symbol<ValueType>(), *expr);
}

} // namespace numsim::cas
#endif // SCALAR_FUNCTIONS_H
