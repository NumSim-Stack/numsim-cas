#ifndef SIMPLIFIER_POW_H
#define SIMPLIFIER_POW_H

#include <cmath>
#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/domain_traits.h>
#include <numsim_cas/core/scalar_number.h>
#include <optional>
#include <variant>

namespace numsim::cas {
namespace detail {

// Exact integer value of a numeric exponent (int64 or whole double).
inline std::optional<std::int64_t>
pow_integer_exponent(scalar_number const &v) {
  if (auto const *i = std::get_if<std::int64_t>(&v.raw()))
    return *i;
  if (auto const *d = std::get_if<double>(&v.raw())) {
    if (*d == std::round(*d) && std::abs(*d) < 9.007199254740992e15)
      return static_cast<std::int64_t>(*d);
  }
  return std::nullopt;
}

// (x^a)^b = x^(a·b) holds for every real x only when a and b are integers:
// a fractional exponent can change the value ((x²)^(1/2) is |x|) or leave the
// reals ((x^(1/2))² is undefined for x < 0). A nonnegative base makes the fold
// sound for any exponents.
template <typename Traits>
bool numeric_integer_exponent(typename Traits::expr_holder_t const &e) {
  auto v = Traits::try_numeric(e);
  return v.has_value() && pow_integer_exponent(*v).has_value();
}

// pow(sqrt(x), n) = pow(x, n/2) agrees for every real x when n is odd: n/2
// keeps a denominator of 2, so the right side leaves the reals for x < 0 just
// as sqrt does. An even n would make it finite there.
template <typename Traits>
bool odd_numeric_exponent(typename Traits::expr_holder_t const &e) {
  auto v = Traits::try_numeric(e);
  if (!v.has_value())
    return false;
  auto const i = pow_integer_exponent(*v);
  return i.has_value() && (*i % 2 != 0);
}

template <typename Traits>
bool nonnegative_numeric_base(typename Traits::expr_holder_t const &base) {
  auto v = Traits::try_numeric(base);
  return v.has_value() && !numeric_less(*v, scalar_number{0});
}

template <typename Traits>
bool pow_exponents_compose(typename Traits::expr_holder_t const &base,
                           typename Traits::expr_holder_t const &inner_exp,
                           typename Traits::expr_holder_t const &outer_exp) {
  return nonnegative_numeric_base<Traits>(base) ||
         (numeric_integer_exponent<Traits>(inner_exp) &&
          numeric_integer_exponent<Traits>(outer_exp));
}

// Extracting pow(z,c) out of pow(x*pow(z,c), n) composes c with n, so each
// nested factor needs the same real-domain check.
template <typename Traits, typename PowList>
bool pow_factors_compose(PowList const &pows,
                         typename Traits::expr_holder_t const &outer_exp) {
  for (auto const &entry : pows) {
    auto const &p = entry.template get<typename Traits::pow_type>();
    if (!pow_exponents_compose<Traits>(p.expr_lhs(), p.expr_rhs(), outer_exp))
      return false;
  }
  return true;
}

//==============================================================================
// pow_dispatch<Traits, Derived> — Base algorithm for pow(A, B)
//==============================================================================
template <typename Traits, typename Derived = void>
requires arithmetic_expression_domain<typename Traits::expression_type>
class pow_dispatch {
public:
  using expr_holder_t = typename Traits::expr_holder_t;

  pow_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

  expr_holder_t get_default() {
    using negative_type = typename Traits::negative_type;
    using pow_type = typename Traits::pow_type;

    // pow(-expr, p): sign pull-out only for provably integer exponents
    // (pow(x, -y) is NOT x/y — the former division-style rules here were
    // unsound, #344)
    if (auto expr_neg{is_same_r<negative_type>(m_lhs)}) {
      if (auto val = Traits::try_numeric(m_rhs)) {
        if (auto n = pow_integer_exponent(*val)) {
          // Recurse through pow() so nested bases keep canonicalizing
          // (pow(-pow(x,2),2) must reach pow(x,4); review on #344).
          // Numeric bases stay structural: pow(2,-1) must keep printing
          // as a division, not fold to the rational 1/2.
          auto inner{expr_neg->get().expr()};
          auto p{Traits::try_numeric(inner)
                     ? make_expression<pow_type>(std::move(inner),
                                                 std::move(m_rhs))
                     : pow(std::move(inner), std::move(m_rhs))};
          return (*n % 2 == 0) ? p : -p;
        }
      }
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
requires arithmetic_expression_domain<typename Traits::expression_type>
class pow_pow_dispatch : public pow_dispatch<Traits, pow_pow_dispatch<Traits>> {
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
    return compose();
  }

  expr_holder_t dispatch(typename Traits::negative_type const &) {
    return compose();
  }

protected:
  expr_holder_t compose() {
    if (!pow_exponents_compose<Traits>(lhs.expr_lhs(), lhs.expr_rhs(),
                                       this->m_rhs))
      return this->get_default();
    return pow(lhs.expr_lhs(), lhs.expr_rhs() * this->m_rhs);
  }

  typename Traits::pow_type const &lhs;
};

//==============================================================================
// mul_pow_dispatch<Traits> — LHS is mul
//==============================================================================
template <typename Traits>
requires arithmetic_expression_domain<typename Traits::expression_type>
class mul_pow_dispatch : public pow_dispatch<Traits, mul_pow_dispatch<Traits>> {
  using base = pow_dispatch<Traits, mul_pow_dispatch<Traits>>;

public:
  using expr_holder_t = typename Traits::expr_holder_t;
  using base::dispatch;
  using base::get_default;

  mul_pow_dispatch(expr_holder_t lhs_in, expr_holder_t rhs)
      : base(std::move(lhs_in), std::move(rhs)),
        lhs{base::m_lhs.template get<typename Traits::mul_type>()} {}

  // (a*b)^n = a^n * b^n only holds for all reals when n is an integer (#345)
  [[nodiscard]] bool integer_outer_exponent() const {
    auto val = Traits::try_numeric(this->m_rhs);
    return val && pow_integer_exponent(*val).has_value();
  }

  // pow(mul, -rhs)
  expr_holder_t dispatch(typename Traits::negative_type const &rhs) {
    auto inner_val = Traits::try_numeric(rhs.expr());
    const bool int_exp{integer_outer_exponent() ||
                       (inner_val && pow_integer_exponent(*inner_val))};
    auto mul_expr{make_expression<typename Traits::mul_type>(lhs)};
    auto &mul{mul_expr.template get<typename Traits::mul_type>()};
    // pow(x*y*pow(z,base), rhs) --> pow(x*y, rhs) * pow(z,base*rhs)
    const auto pows{get_all<typename Traits::pow_type>(lhs)};
    if (!pows.empty() && int_exp &&
        pow_factors_compose<Traits>(pows, this->m_rhs)) {
      expr_holder_t result;
      for (const auto &expr : pows) {
        const auto &pow_expr{expr.template get<typename Traits::pow_type>()};
        mul.symbol_map().erase(expr);
        mul.invalidate_hash();
        auto pow_n{pow(pow_expr.expr_lhs(), pow_expr.expr_rhs() * this->m_rhs)};
        if (!result.is_valid()) {
          result = std::move(pow_n);
        } else {
          result = result * std::move(pow_n);
        }
      }
      // If mul has only coeff left (no children), collapse to coeff
      if (mul.symbol_map().empty()) {
        if (mul.coeff().is_valid())
          return pow(mul.coeff(), this->m_rhs) * result;
        return result;
      }
      // single remaining child: pow(mul{y}, n) is non-canonical and never
      // cancels against pow(y, n) (round-2 review on #345)
      if (mul.symbol_map().size() == 1 && !mul.coeff().is_valid()) {
        return pow(mul.symbol_map().begin()->second, this->m_rhs) * result;
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
    if (!pows.empty() && integer_outer_exponent() &&
        pow_factors_compose<Traits>(pows, this->m_rhs)) {
      expr_holder_t result;
      for (const auto &expr : pows) {
        const auto &pow_expr{expr.template get<typename Traits::pow_type>()};
        mul.symbol_map().erase(expr);
        mul.invalidate_hash();
        auto pow_n{pow(pow_expr.expr_lhs(), pow_expr.expr_rhs() * this->m_rhs)};
        if (!result.is_valid()) {
          result = std::move(pow_n);
        } else {
          result = result * std::move(pow_n);
        }
      }
      // If mul has only coeff left (no children), collapse to coeff
      if (mul.symbol_map().empty()) {
        if (mul.coeff().is_valid())
          return pow(mul.coeff(), this->m_rhs) * result;
        return result;
      }
      // single remaining child: pow(mul{y}, n) is non-canonical and never
      // cancels against pow(y, n) (round-2 review on #345)
      if (mul.symbol_map().size() == 1 && !mul.coeff().is_valid()) {
        return pow(mul.symbol_map().begin()->second, this->m_rhs) * result;
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
