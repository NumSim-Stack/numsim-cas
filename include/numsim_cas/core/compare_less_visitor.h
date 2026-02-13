#ifndef COMPARE_LESS_VISITOR_H
#define COMPARE_LESS_VISITOR_H

#include <numsim_cas/expression_holder.h>

namespace numsim::cas {

class compare_less_pretty_print_base {
public:
  compare_less_pretty_print_base() {}

  template <typename LHS, typename RHS>
  constexpr inline bool dispatch(LHS const &lhs,
                                 RHS const &rhs) const noexcept {
    if (lhs.hash_value() != rhs.hash_value())
      return lhs.hash_value() < rhs.hash_value();
    if (lhs.id() != rhs.id())
      return lhs.id() < rhs.id();
    return false;
  }

  template <typename Type>
  constexpr inline bool dispatch(Type const &lhs,
                                 Type const &rhs) const noexcept {
    return lhs < rhs;
  }

  template <typename BaseTree, typename DerivedSymbol>
  constexpr inline bool
  dispatch(n_ary_tree<BaseTree> const &lhs,
           symbol_base<DerivedSymbol> const &rhs) const noexcept {
    if (lhs.hash_map().size() == 1) {
      return lhs.hash_map().begin()->second < rhs;
    }
    return default_compare(lhs, rhs);
  }

  template <typename BaseTree, typename BaseSymbol>
  constexpr inline bool
  dispatch(symbol_base<BaseSymbol> const &lhs,
           n_ary_tree<BaseTree> const &rhs) const noexcept {
    if (rhs.hash_map().size() == 1) {
      return lhs < rhs.hash_map().begin()->second;
    }
    return default_compare(lhs, rhs);
  }
};

} // namespace numsim::cas

#endif // COMPARE_LESS_VISITOR_H
