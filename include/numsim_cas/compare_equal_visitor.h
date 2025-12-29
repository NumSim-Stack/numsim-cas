#ifndef COMPARE_EQUAL_VISITOR_H
#define COMPARE_EQUAL_VISITOR_H

#include "numsim_cas_type_traits.h"
#include <variant>

namespace numsim::cas {
class compare_equal_visitor {
public:
  compare_equal_visitor() = default;

  template <typename Type>
  constexpr inline auto operator()(expression_holder<Type> const &lhs,
                                   expression_holder<Type> const &rhs) {
    return std::visit(*this, *lhs, *rhs);
  }

  template <typename LHS, typename RHS>
  constexpr inline auto operator()([[maybe_unused]] LHS const &lhs,
                                   [[maybe_unused]] RHS const &rhs) {
    return false;
  }

  template <typename Type>
  constexpr inline auto operator()(Type const &lhs, Type const &rhs) {
    return lhs == rhs;
  }

  template <typename Type>
  constexpr inline auto operator()(scalar_zero<Type> const &,
                                   scalar_zero<Type> const &) {
    return true;
  }
};
} // namespace numsim::cas

#endif // COMPARE_EQUAL_VISITOR_H
