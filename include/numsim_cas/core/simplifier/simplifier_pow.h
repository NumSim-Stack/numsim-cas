#ifndef SIMPLIFIER_POW_H
#define SIMPLIFIER_POW_H

#include <numsim_cas/basic_functions.h>

namespace numsim::cas {
namespace detail {

//==============================================================================
// pow_dispatch<Traits, Derived> — Base algorithm for pow(A, B)
//==============================================================================
template <typename Traits, typename Derived = void> class pow_dispatch {
public:
  using expr_holder_t = typename Traits::expr_holder_t;

  pow_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

  expr_holder_t get_default() {
    using negative_type = typename Traits::negative_type;
    using mul_type = typename Traits::mul_type;
    using pow_type = typename Traits::pow_type;

    if (is_same<negative_type>(m_rhs)) {
      const auto expr{m_rhs.template get<negative_type>().expr()};
      // expr / expr --> 1; for x /= 0
      if (m_lhs == expr) {
        return Traits::one();
      }

      // expr*x / x --> expr; for x /= 0
      if (is_same<mul_type>(m_lhs)) {
        const auto &map{m_lhs.template get<mul_type>().hash_map()};
        auto pos{map.find(expr)};
        if (pos != map.end()) {
          auto copy{make_expression<mul_type>(m_lhs.template get<mul_type>())};
          copy.template get<mul_type>().hash_map().erase(expr);
          return copy;
        }
      }
    }
    // pow(-expr, power) --> -pow(expr, power)
    if (auto expr_neg{is_same_r<negative_type>(m_lhs)}) {
      return -make_expression<pow_type>(expr_neg->get().expr(),
                                        std::move(m_rhs));
    }
    return make_expression<pow_type>(std::move(m_lhs), std::move(m_rhs));
  }

  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

protected:
  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

//==============================================================================
// pow_pow_dispatch<Traits> — LHS is pow: pow(pow(x,a),b) → pow(x,a*b)
//==============================================================================
template <typename Traits>
class pow_pow_dispatch
    : public pow_dispatch<Traits, pow_pow_dispatch<Traits>> {
  using base = pow_dispatch<Traits, pow_pow_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;
  using base::get_default;

  pow_pow_dispatch(expr_holder_t lhs_in, expr_holder_t rhs)
      : base(std::move(lhs_in), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::pow_type>()} {}

  /// pow(pow(x,a),b) --> pow(x,a*b)
  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    return pow(lhs.expr_lhs(), lhs.expr_rhs() * this->m_rhs);
  }

  expr_holder_t dispatch(typename Traits::negative_type const &) {
    return pow(lhs.expr_lhs(), lhs.expr_rhs() * this->m_rhs);
  }

protected:
  typename Traits::pow_type const &lhs;
};

//==============================================================================
// mul_pow_dispatch<Traits> — LHS is mul
//==============================================================================
template <typename Traits>
class mul_pow_dispatch
    : public pow_dispatch<Traits, mul_pow_dispatch<Traits>> {
  using base = pow_dispatch<Traits, mul_pow_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;
  using base::get_default;

  mul_pow_dispatch(expr_holder_t lhs_in, expr_holder_t rhs)
      : base(std::move(lhs_in), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::mul_type>()} {}

  // pow(mul, -rhs)
  expr_holder_t dispatch(typename Traits::negative_type const &rhs) {
    auto pos{lhs.hash_map().find(rhs.expr())};
    auto mul_expr{make_expression<typename Traits::mul_type>(lhs)};
    auto &mul{mul_expr.template get<typename Traits::mul_type>()};
    // x*y*z / x --> y*z
    if (pos != lhs.hash_map().end()) {
      mul.hash_map().erase(rhs.expr());
      return mul_expr;
    }

    // pow(x*y*pow(z,base), rhs) --> pow(x*y, rhs) * pow(z,base*rhs)
    const auto pows{get_all<typename Traits::pow_type>(lhs)};
    if (!pows.empty()) {
      expr_holder_t result;
      for (const auto &expr : pows) {
        const auto &pow_expr{
            expr.template get<typename Traits::pow_type>()};
        mul.hash_map().erase(expr);
        auto pow_n{pow(pow_expr.expr_lhs(),
                       pow_expr.expr_rhs() * this->m_rhs)};
        if (!result.is_valid()) {
          result = std::move(pow_n);
        } else {
          result = result * std::move(pow_n);
        }
      }
      // If mul has only coeff left (no children), collapse to coeff
      if (mul.hash_map().empty()) {
        if (mul.coeff().is_valid())
          return pow(mul.coeff(), this->m_rhs) * result;
        return result;
      }
      return pow(mul_expr, this->m_rhs) * result;
    }

    return this->get_default();
  }

  template <typename Expr>
  expr_holder_t dispatch([[maybe_unused]] Expr const &rhs) {
    auto mul_expr{make_expression<typename Traits::mul_type>(lhs)};
    auto &mul{mul_expr.template get<typename Traits::mul_type>()};

    // pow(x*y*pow(z,base), rhs) --> pow(x*y, rhs) * pow(z,base*rhs)
    const auto pows{get_all<typename Traits::pow_type>(lhs)};
    if (!pows.empty()) {
      expr_holder_t result;
      for (const auto &expr : pows) {
        const auto &pow_expr{
            expr.template get<typename Traits::pow_type>()};
        mul.hash_map().erase(expr);
        auto pow_n{pow(pow_expr.expr_lhs(),
                       pow_expr.expr_rhs() * this->m_rhs)};
        if (!result.is_valid()) {
          result = std::move(pow_n);
        } else {
          result = result * std::move(pow_n);
        }
      }
      // If mul has only coeff left (no children), collapse to coeff
      if (mul.hash_map().empty()) {
        if (mul.coeff().is_valid())
          return pow(mul.coeff(), this->m_rhs) * result;
        return result;
      }
      return pow(mul_expr, this->m_rhs) * result;
    }

    return this->get_default();
  }

protected:
  typename Traits::mul_type const &lhs;
};

} // namespace detail
} // namespace numsim::cas

#endif // SIMPLIFIER_POW_H
