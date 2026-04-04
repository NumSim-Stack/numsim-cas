#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <numsim_cas/core/n_ary_tree.h>
#include <numsim_cas/core/operators.h>

namespace numsim::cas {

/// merge two n_ary_trees
///   --> use std::function for special handling?
///   --> add (x+y+z) + (x+a) --> 2*x+y+z+a --> mul
///   --> mul (x*y*z) * (x*a) --> pow(x,2)*y*z*a --> pow
template <typename Derived>
constexpr inline void merge_add(n_ary_tree<Derived> const &lhs,
                                n_ary_tree<Derived> const &rhs,
                                n_ary_tree<Derived> &result) {
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

  // Helper: insert into result, combining with existing entry if present.
  // After combining child+rhs_child (e.g. x+x=2*x), the result may
  // match another entry already in the result (e.g. existing 2*x).
  auto safe_push = [&](expression_holder<expr_t> entry) {
    auto it = result.symbol_map().find(entry);
    if (it != result.symbol_map().end()) {
      auto combined = it->second + entry;
      result.symbol_map().erase(it);
      result.push_back(std::move(combined));
    } else {
      result.push_back(std::move(entry));
    }
  };

  expr_set<expression_holder<expr_t>> used_expr;
  for (auto &child : lhs.symbol_map() | std::views::values) {
    auto pos{rhs.symbol_map().find(child)};
    if (pos != rhs.symbol_map().end()) {
      used_expr.insert(pos->second);
      safe_push(child + pos->second);
    } else {
      safe_push(child);
    }
  }
  if (used_expr.size() != rhs.size()) {
    for (auto &child : rhs.symbol_map() | std::views::values) {
      if (!used_expr.count(child)) {
        safe_push(child);
      }
    }
  }
}

} // namespace numsim::cas

#endif // FUNCTIONS_H
