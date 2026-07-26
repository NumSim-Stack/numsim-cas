#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <numsim_cas/core/n_ary_tree.h>
#include <numsim_cas/core/operators.h>

namespace numsim::cas {

/// Insert `entry` into an add-semantics tree, combining with an existing
/// exact match or exact negation until no collision remains; `is_zero`
/// filters fully-cancelled results so {t, -t} pairs never coexist
/// (round-8 review: such a pair downstream caused a double-consume merge).
template <typename Derived, typename IsZero>
constexpr inline void
add_insert_signed(n_ary_tree<Derived> &tree,
                  expression_holder<typename Derived::expr_t> entry,
                  IsZero &&is_zero) {
  while (entry.is_valid() && !is_zero(entry)) {
    auto pos = tree.find_like(entry);
    if (pos == tree.symbol_map().end()) {
      auto neg{-entry};
      pos = tree.find_like(neg);
      if (pos != tree.symbol_map().end() && !(pos->second == neg)) {
        pos = tree.symbol_map().end();
      }
    }
    if (pos == tree.symbol_map().end()) {
      tree.merge_or_insert(std::move(entry));
      return;
    }
    auto next = pos->second + entry;
    tree.symbol_map().erase(pos);
    entry = std::move(next);
  }
}

/// merge two n_ary_trees
///   --> add (x+y+z) + (x+a) --> 2*x+y+z+a --> mul
/// `is_zero` filters fully-cancelled combines (x + (-x)); negation pairs
/// share no hash, so a failed find_like retries with -child (round-7).
/// A matched rhs child is consumed exactly once: a second lhs child must
/// not re-match it (round-8 review: 5x and -(5x) both consuming -(5x)
/// turned y-5x into y-10x).
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
    if (pos != rhs.symbol_map().end() && used_expr.count(pos->second)) {
      pos = rhs.symbol_map().end();
    }
    if (pos == rhs.symbol_map().end()) {
      // only an exact -child cancels; like-terms of -child would nest adds
      auto neg_child{-child};
      pos = rhs.find_like(neg_child);
      if (pos != rhs.symbol_map().end() &&
          (!(pos->second == neg_child) || used_expr.count(pos->second))) {
        pos = rhs.symbol_map().end();
      }
    }
    if (pos != rhs.symbol_map().end()) {
      used_expr.insert(pos->second);
      auto combined{child + pos->second};
      if (!is_zero(combined)) {
        add_insert_signed(result, std::move(combined), is_zero);
      }
    } else {
      add_insert_signed(result, child, is_zero);
    }
  }
  if (used_expr.size() != rhs.size()) {
    for (auto &child : rhs.symbol_map() | std::views::values) {
      if (!used_expr.count(child)) {
        add_insert_signed(result, child, is_zero);
      }
    }
  }
}

} // namespace numsim::cas

#endif // FUNCTIONS_H
