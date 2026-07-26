#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <numsim_cas/core/n_ary_tree.h>
#include <numsim_cas/core/operators.h>

namespace numsim::cas {

/// merge two n_ary_trees
///   --> add (x+y+z) + (x+a) --> 2*x+y+z+a --> mul
/// `is_zero` filters fully-cancelled combines (x + (-x)); negation pairs
/// share no hash, so a failed find_like retries with -child (round-7).
template <typename Derived, typename IsZero>
constexpr inline void merge_add(n_ary_tree<Derived> const &lhs,
                                n_ary_tree<Derived> const &rhs,
                                n_ary_tree<Derived> &result, IsZero &&is_zero) {
  using expr_t = typename Derived::expr_t;

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

  expr_set<expression_holder<expr_t>> used_expr;
  for (auto &child : lhs.symbol_map() | std::views::values) {
    auto pos{rhs.find_like(child)};
    if (pos == rhs.symbol_map().end()) {
      // only an exact -child cancels; like-terms of -child would nest adds
      auto neg_child{-child};
      pos = rhs.find_like(neg_child);
      if (pos != rhs.symbol_map().end() && !(pos->second == neg_child)) {
        pos = rhs.symbol_map().end();
      }
    }
    if (pos != rhs.symbol_map().end()) {
      used_expr.insert(pos->second);
      auto combined{child + pos->second};
      if (!is_zero(combined)) {
        result.merge_or_insert(std::move(combined));
      }
    } else {
      result.merge_or_insert(child);
    }
  }
  if (used_expr.size() != rhs.size()) {
    for (auto &child : rhs.symbol_map() | std::views::values) {
      if (!used_expr.count(child)) {
        result.merge_or_insert(child);
      }
    }
  }
}

template <typename Derived>
constexpr inline void merge_add(n_ary_tree<Derived> const &lhs,
                                n_ary_tree<Derived> const &rhs,
                                n_ary_tree<Derived> &result) {
  merge_add(lhs, rhs, result, [](auto const &) { return false; });
}

} // namespace numsim::cas

#endif // FUNCTIONS_H
