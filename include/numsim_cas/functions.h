#ifndef FUNCTIONS_H
#define FUNCTIONS_H

// #include "scalar_functions.h"
// #include "tensor_functions.h"
#include "numsim_cas_type_traits.h"
#include "scalar/visitors/scalar_differentiation.h"
#include "scalar/visitors/scalar_evaluator.h"
#include "tensor/visitors/tensor_evaluator.h"

namespace numsim::cas {

template <typename ValueType>
inline auto eval(expression_holder<tensor_expression<ValueType>> expr) {
  tensor_evaluator<ValueType> eval;
  return eval.apply(expr);
}

template <typename ValueType>
inline auto diff(expression_holder<scalar_expression<ValueType>> &expr,
                 expression_holder<scalar_expression<ValueType>> const &arg) {
  scalar_differentiation<ValueType> visitor(arg);
  return visitor.apply(expr);
}

template <typename ValueType>
inline auto diff(expression_holder<scalar_expression<ValueType>> &&expr,
                 expression_holder<scalar_expression<ValueType>> const &arg) {
  scalar_differentiation<ValueType> visitor(arg);
  return visitor.apply(std::move(expr));
}

template <typename ValueType>
inline auto diff(expression_holder<tensor_expression<ValueType>> expr) {
  tensor_evaluator<ValueType> eval;
  return eval.apply(expr);
}

template <typename ValueType>
inline auto eval(scalar_expression<ValueType> &expr) {
  scalar_evaluator<ValueType> eval;
  return eval.apply(expr);
}

template <typename ValueType>
inline auto eval(expression_holder<scalar_expression<ValueType>> &&expr) {
  scalar_evaluator<ValueType> eval;
  return eval.apply(expr);
}

template <typename T, std::size_t Dim, std::size_t Rank>
inline auto get_tensor(std::unique_ptr<tensor_data_base<T>> const &data) {
  return static_cast<tensor_data<T, Dim, Rank> &>(*data.get()).data();
}

/// merge two n_ary_trees
///   --> use std::function for special handling?
///   --> add (x+y+z) + (x+a) --> 2*x+y+z+a --> mul
///   --> mul (x*y*z) * (x*a) --> pow(x,2)*y*z*a --> pow
template <typename Derived, typename ExprType>
constexpr inline auto merge_add(n_ary_tree<ExprType, Derived> const &lhs,
                                n_ary_tree<ExprType, Derived> const &rhs,
                                n_ary_tree<ExprType, Derived> &result) {
  if (lhs.coeff().is_valid() && rhs.coeff().is_valid()) {
    result.set_coeff(lhs.coeff() + rhs.coeff());
  } else {
    if (lhs.coeff().is_valid()) {
      result.set_coeff(lhs.coeff());
    }
    if (rhs.coeff().is_valid()) {
      result.set_coeff(rhs.coeff());
    }
  }

  expr_set<expression_holder<ExprType>> used_expr;
  for (auto &child : lhs.hash_map() | std::views::values) {
    auto pos{rhs.hash_map().find(child.get().hash_value())};
    if (pos != rhs.hash_map().end()) {
      used_expr.insert(pos->second);
      result.push_back(child + pos->second);
    } else {
      result.push_back(child);
    }
  }
  if (used_expr.size() != rhs.size()) {
    for (auto &child : rhs.hash_map() | std::views::values) {
      if (!used_expr.count(child)) {
        result.push_back(child);
      }
    }
  }
}

} // namespace numsim::cas

#endif // FUNCTIONS_H
