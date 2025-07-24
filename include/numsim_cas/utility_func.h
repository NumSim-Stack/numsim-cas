#ifndef UTILITY_FUNC_H
#define UTILITY_FUNC_H

#include "numsim_cas_forward.h"
#include "numsim_cas_type_traits.h"
#include <string>
#include <tuple>

// #include "expression.h"

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

template <typename ValueType>
class compare_types_imp<scalar_expression<ValueType>> final
    : public compare_types_base_imp<scalar_expression<ValueType>> {
public:
  using expr_t = scalar_expression<ValueType>;
  using base_t = compare_types_base_imp<scalar_expression<ValueType>>;
  compare_types_imp(expression_holder<expr_t> const &lhs,
                    expression_holder<expr_t> const &rhs)
      : base_t(lhs, rhs) {}

  using base_t::operator();
  using base_t::compare;

  template <typename Base>
  constexpr inline auto operator()(scalar_add<Base> const &lhs,
                                   scalar<Base> const &rhs) const {
    return compare(lhs, rhs);
  }

  template <typename Base>
  constexpr inline auto operator()(scalar<Base> const &lhs,
                                   scalar_add<Base> const &rhs) const {
    return compare(lhs, rhs);
  }

  template <typename Base>
  constexpr inline auto operator()(scalar_mul<Base> const &lhs,
                                   scalar<Base> const &rhs) const {
    return compare(lhs, rhs);
  }

  template <typename Base>
  constexpr inline auto operator()(scalar<Base> const &lhs,
                                   scalar_mul<Base> const &rhs) const {
    return compare(lhs, rhs);
  }

  template <typename Base>
  constexpr inline auto
  operator()(scalar_pow<Base> const &lhs,
             [[maybe_unused]] scalar<Base> const &rhs) const {
    return lhs.expr_lhs() < m_rhs;
  }

  template <typename Base>
  constexpr inline auto operator()([[maybe_unused]] scalar<Base> const &lhs,
                                   scalar_pow<Base> const &rhs) const {
    return m_lhs < rhs.expr_lhs();
  }

  using base_t::m_lhs;
  using base_t::m_rhs;
};

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
    return lhs.get().hash_value() < lhs.get().hash_value();
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

namespace numsim::cas {

// Helper function for combining hashes
template <typename T>
inline void hash_combine(std::size_t &seed, const T &value) {
  // std::hash<T> hasher;
  seed ^= static_cast<std::size_t>(value) +
          static_cast<std::size_t>(0x9e3779b9) + (seed << 6) + (seed >> 2);
}

inline void hash_combine(std::size_t &seed, const std::string &value) {
  for (const auto &c : value) {
    hash_combine(seed, c);
  }
}

template <typename... Args> constexpr inline auto tuple(Args &&...args) {
  return std::make_tuple(std::forward<Args>(args)...);
}

template <typename BaseExpr>
constexpr inline const auto &
get(expression_holder<BaseExpr> const &expr_holder) {
  return std::visit([](const auto &expr) { return std::ref(expr); },
                    *expr_holder)
      .get();
}

} // namespace numsim::cas

#endif // UTILITY_FUNC_H
