#ifndef SCALAR_SIMPLIFIER_MUL_H
#define SCALAR_SIMPLIFIER_MUL_H

#include "../../numsim_cas_forward.h"
#include "../scalar_constant.h"
#include <type_traits>

namespace numsim::cas {

namespace simplifier {
template <typename ExprLHS, typename ExprRHS> class mul_default {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;

  mul_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  template <typename Expr> constexpr inline expr_type operator()(Expr const &) {
    return get_default();
  }

  // expr_lhs * (-expr_rhs) -->  -(expr_lhs * expr_rhs)
  constexpr inline expr_type
  operator()(scalar_negative<value_type> const &rhs) {
    return -(std::forward<ExprLHS>(m_lhs) * rhs.expr());
  }

  // expr * zero --> zero
  constexpr inline expr_type operator()(scalar_zero<value_type> const &) {
    return m_rhs;
  }

  // expr * 1 --> expr
  constexpr inline expr_type operator()(scalar_one<value_type> const &) {
    return m_lhs;
  }

protected:
  expr_type get_default() {
    if (m_lhs.get().hash_value() == m_rhs.get().hash_value()) {
      return std::pow(m_lhs, 2);
    }
    const auto lhs_constant{is_same<scalar_constant<value_type>>(m_lhs)};
    const auto rhs_constant{is_same<scalar_constant<value_type>>(m_rhs)};
    auto mul_new{make_expression<scalar_mul<value_type>>()};
    auto &mul{mul_new.template get<scalar_mul<value_type>>()};
    if (lhs_constant) {
      mul.set_coeff(m_lhs);
    } else {
      mul.push_back(m_lhs);
    }

    if (rhs_constant) {
      mul.set_coeff(m_rhs);
    } else {
      mul.push_back(m_rhs);
    }
    return std::move(mul_new);
  }

  template <typename _Expr, typename _ValueType>
  constexpr auto get_coefficient(_Expr const &expr, _ValueType const &value) {
    if constexpr (is_detected_v<has_coefficient, _Expr>) {
      auto func{[&](auto const &coeff) {
        return coeff.is_valid()
                   ? coeff.template get<scalar_constant<value_type>>()()
                   : value;
      }};
      return func(expr.coeff());
    }
    return value;
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class constant_mul final : public mul_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = mul_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_coefficient;
  using base::m_rhs;

  constant_mul(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_constant<value_type>>()} {}

  constexpr inline expr_type
  operator()(scalar_constant<value_type> const &rhs) {
    const auto value{lhs() * rhs()};
    return make_expression<scalar_constant<value_type>>(value);
  }

  constexpr inline expr_type
  operator()([[maybe_unused]] scalar_mul<value_type> const &rhs) {
    auto mul_expr{make_expression<scalar_mul<value_type>>(rhs)};
    auto &mul{mul_expr.template get<scalar_mul<value_type>>()};
    auto coeff{get_coefficient(mul, static_cast<value_type>(1)) * base::m_lhs};
    mul.set_coeff(std::move(coeff));
    return std::move(mul_expr);
  }

  template <typename ExprType>
  constexpr inline expr_type operator()([[maybe_unused]] ExprType const &rhs) {
    if (lhs() == static_cast<value_type>(1)) {
      return std::forward<ExprRHS>(m_rhs);
    }
    return base::get_default();
  }

private:
  scalar_constant<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class n_ary_mul final : public mul_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = mul_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_coefficient;

  n_ary_mul(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_mul<value_type>>()} {}

  // expr * constant
  constexpr inline expr_type
  operator()([[maybe_unused]] scalar_constant<value_type> const &rhs) {
    auto mul_expr{make_expression<scalar_mul<value_type>>(lhs)};
    auto &mul{mul_expr.template get<scalar_mul<value_type>>()};
    auto coeff{mul.coeff() * m_rhs};
    mul.set_coeff(std::move(coeff));
    return std::move(mul_expr);
  }

  auto operator()([[maybe_unused]] scalar<value_type> const &rhs) {
    /// do a deep copy of data
    auto expr_mul{make_expression<scalar_mul<value_type>>(lhs)};
    auto &mul{expr_mul.template get<scalar_mul<value_type>>()};
    /// check if sub_exp == expr_rhs for sub_exp \in expr_lhs
    auto pos{lhs.hash_map().find(rhs.hash_value())};
    if (pos != lhs.hash_map().end()) {
      auto expr{binary_scalar_mul_simplify(pos->second, m_rhs)};
      mul.hash_map().erase(rhs.hash_value());
      mul.push_back(expr);
      return expr_mul;
    }
    /// no equal expr or sub_expr
    mul.push_back(m_rhs);
    return expr_mul;
  }

  template <typename Expr> auto operator()([[maybe_unused]] Expr const &rhs) {
    auto expr_mul{make_expression<scalar_mul<value_type>>(lhs)};
    auto &mul{expr_mul.template get<scalar_mul<value_type>>()};
    mul.push_back(m_rhs);
    return expr_mul;
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_mul<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class scalar_pow_mul final : public mul_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = mul_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_coefficient;
  using base::get_default;

  scalar_pow_mul(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_pow<value_type>>()} {}

  auto operator()([[maybe_unused]] scalar<value_type> const &rhs) {
    if (lhs.expr_lhs().get().hash_value() == rhs.hash_value()) {
      const auto rhs_expr{lhs.expr_rhs() +
                          make_expression<scalar_one<value_type>>()};
      return make_expression<scalar_pow<value_type>>(lhs.expr_lhs(),
                                                     std::move(rhs_expr));
    }
    return get_default();
  }

  auto operator()([[maybe_unused]] scalar_pow<value_type> const &rhs) {
    if (lhs.hash_value() == rhs.hash_value()) {
      const auto rhs_expr{lhs.expr_rhs() + rhs.expr_rhs()};
      return make_expression<scalar_pow<value_type>>(lhs.expr_lhs(),
                                                     std::move(rhs_expr));
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_pow<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class symbol_mul final : public mul_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = mul_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_coefficient;
  using base::get_default;

  symbol_mul(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar<value_type>>()} {}

  /// x*x --> pow(x,2)
  constexpr inline expr_type operator()(scalar<value_type> const &rhs) {
    if (&lhs == &rhs) {
      return make_expression<scalar_pow<value_type>>(
          std::forward<ExprRHS>(m_rhs),
          make_expression<scalar_constant<value_type>>(2));
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS> struct mul_base {
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;

  mul_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  constexpr inline expr_type
  operator()(scalar_negative<value_type> const &lhs) {
    auto expr{lhs.expr() * m_rhs};
    return -expr;
  }

  constexpr inline expr_type operator()(scalar_constant<value_type> const &) {
    return visit(constant_mul<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  constexpr inline expr_type operator()(scalar_mul<value_type> const &) {
    return visit(n_ary_mul<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                             std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  constexpr inline expr_type operator()(scalar<value_type> const &) {
    return visit(symbol_mul<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                              std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  constexpr inline expr_type operator()(scalar_pow<value_type> const &) {
    return visit(scalar_pow_mul<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                  std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  // zero * expr --> zero
  constexpr inline expr_type operator()(scalar_zero<value_type> const &) {
    return m_lhs;
  }

  // one * expr --> expr
  constexpr inline expr_type operator()(scalar_one<value_type> const &) {
    return m_rhs;
  }

  template <typename Type> constexpr inline expr_type operator()(Type const &) {
    return visit(mul_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                               std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

} // namespace simplifier
} // namespace numsim::cas
#endif // SCALAR_SIMPLIFIER_MUL_H
