#ifndef COMPARE_LESS_VISITOR_H
#define COMPARE_LESS_VISITOR_H

#include "numsim_cas_type_traits.h"
#include <concepts>
#include <variant>

namespace numsim::cas {
namespace detail {

template <typename LHS, typename RHS>
constexpr bool has_less_operator = requires(LHS a, RHS b) {
  { a < b } -> std::convertible_to<bool>;
};

template <typename ExprType> class compare_types_base_imp {
public:
  using expr_t = ExprType;
  compare_types_base_imp(expression_holder<expr_t> const &lhs,
                         expression_holder<expr_t> const &rhs)
      : m_lhs(lhs), m_rhs(rhs) {}

  template <typename LHS, typename RHS>
  constexpr inline auto operator()(LHS const &lhs, RHS const &rhs) const {
    return default_compare(lhs, rhs);
  }

  template <typename VariantType>
  constexpr inline auto operator()(VariantType const &lhs,
                                   VariantType const &rhs) const {
    return lhs < rhs;
  }

protected:
  template <typename LHS, typename RHS>
  constexpr inline auto default_compare(LHS const &lhs, RHS const &rhs) const {
    return lhs.hash_value() < rhs.hash_value();
  }

  template <typename DerivedTree, typename DerivedSymbol>
  constexpr inline auto
  compare(n_ary_tree<expr_t, DerivedTree> const &lhs,
          symbol_base<expr_t, DerivedSymbol> const &rhs) const {
    if (lhs.hash_map().size() == 1) {
      return lhs.hash_map().begin()->second < m_rhs;
    }
    return default_compare(lhs, rhs);
  }

  template <typename DerivedTree, typename DerivedSymbol>
  constexpr inline auto
  compare(symbol_base<expr_t, DerivedSymbol> const &lhs,
          n_ary_tree<expr_t, DerivedTree> const &rhs) const {
    if (rhs.hash_map().size() == 1) {
      return m_lhs < rhs.hash_map().begin()->second;
    }
    return default_compare(lhs, rhs);
  }

  expression_holder<expr_t> const &m_lhs;
  expression_holder<expr_t> const &m_rhs;
};

template <typename ValueType> class compare_types_imp;

template <typename Type> class compare_types_imp {
public:
  compare_types_imp(expression_holder<Type> const &lhs,
                    expression_holder<Type> const &rhs)
      : m_lhs(lhs), m_rhs(rhs) {}

  template <typename LHS, typename RHS>
  constexpr inline auto operator()(LHS const &lhs, RHS const &rhs) const {
    if constexpr (has_less_operator<LHS, RHS>) {
      return lhs < rhs;
    }
    return lhs.hash_value() < rhs.hash_value();
  }

  template <typename VariantType>
  constexpr inline auto operator()(VariantType const &lhs,
                                   VariantType const &rhs) const {
    return lhs < rhs;
  }

private:
  expression_holder<Type> const &m_lhs;
  expression_holder<Type> const &m_rhs;
};
} // namespace detail

class compare_type {
public:
  compare_type() = default;

  template <typename ExprType>
  constexpr inline auto
  operator()(expression_holder<ExprType> const &lhs,
             expression_holder<ExprType> const &rhs) const {
    detail::compare_types_imp<ExprType> visitor(lhs, rhs);
    return std::visit(visitor, *lhs, *rhs);
  }

  template <typename ExprTypeLHS, typename ExprTypeRHS>
  constexpr inline auto
  operator()(expression_holder<ExprTypeLHS> const &lhs,
             expression_holder<ExprTypeRHS> const &rhs) const {
    return lhs.get().hash_value() < rhs.get().hash_value();
  }
};

class equal_type {
public:
  equal_type() = default;

  template <typename Type>
  constexpr inline auto operator()(expression_holder<Type> const &lhs,
                                   expression_holder<Type> const &rhs) {
    return std::visit(*this, *lhs, *rhs);
  }

  template <typename LHS, typename RHS>
  constexpr inline auto operator()(LHS const &lhs, RHS const &rhs) {
    return lhs.hash_value() == rhs.hash_value();
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

#endif // COMPARE_LESS_VISITOR_H
