#ifndef TENSOR_COMPARE_LESS_VISITOR_H
#define TENSOR_COMPARE_LESS_VISITOR_H

#include "../compare_less_visitor.h"

namespace numsim::cas {
namespace detail {

template <typename ValueType>
class compare_types_imp<tensor_expression<ValueType>> final
    : public compare_types_base_imp<tensor_expression<ValueType>> {
public:
  using expr_t = tensor_expression<ValueType>;
  using base_t = compare_types_base_imp<tensor_expression<ValueType>>;
  compare_types_imp(expression_holder<expr_t> const &lhs,
                    expression_holder<expr_t> const &rhs)
      : base_t(lhs, rhs) {}

  using base_t::operator();
  using base_t::compare;

  template <typename Base>
  constexpr inline auto operator()(tensor_add<Base> const &lhs,
                                   tensor<Base> const &rhs) const {
    return compare(lhs, rhs);
  }

  template <typename Base>
  constexpr inline auto operator()(tensor<Base> const &lhs,
                                   tensor_add<Base> const &rhs) const {
    return compare(lhs, rhs);
  }

  template <typename Base>
  constexpr inline auto operator()(tensor_mul<Base> const &lhs,
                                   tensor<Base> const &rhs) const {
    return compare(lhs, rhs);
  }

  template <typename Base>
  constexpr inline auto operator()(tensor<Base> const &lhs,
                                   tensor_mul<Base> const &rhs) const {
    return compare(lhs, rhs);
  }

  template <typename Base>
  constexpr inline auto
  operator()(tensor_pow<Base> const &lhs,
             [[maybe_unused]] tensor<Base> const &rhs) const {
    return lhs.expr_lhs() < m_rhs;
  }

  template <typename Base>
  constexpr inline auto operator()([[maybe_unused]] tensor<Base> const &lhs,
                                   tensor_pow<Base> const &rhs) const {
    return m_lhs < rhs.expr_lhs();
  }

  using base_t::m_lhs;
  using base_t::m_rhs;
};

} // namespace detail
} // namespace numsim::cas

#endif // TENSOR_COMPARE_LESS_VISITOR_H
