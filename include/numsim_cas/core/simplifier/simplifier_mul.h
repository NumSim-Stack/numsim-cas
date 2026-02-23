#ifndef SIMPLIFIER_MUL_H
#define SIMPLIFIER_MUL_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/domain_traits.h>
#include <numsim_cas/core/scalar_number.h>

namespace numsim::cas {
namespace detail {

//==============================================================================
// mul_dispatch<Traits, Derived> — Base algorithm for A * B
//==============================================================================
template <typename Traits, typename Derived = void>
requires arithmetic_expression_domain<typename Traits::expression_type>
class mul_dispatch {
public:
  using expr_holder_t = typename Traits::expr_holder_t;

  mul_dispatch(expr_holder_t lhs, expr_holder_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

  expr_holder_t get_default() {
    using mul_type = typename Traits::mul_type;
    using pow_type = typename Traits::pow_type;

    // Same expression: x * x → pow(x, 2)
    if (m_lhs == m_rhs) {
      return make_expression<pow_type>(m_lhs,
                                       Traits::make_constant(scalar_number{2}));
    }

    auto lhs_val = Traits::try_numeric(m_lhs);
    auto rhs_val = Traits::try_numeric(m_rhs);

    // Both numeric: multiply directly
    if (lhs_val && rhs_val) {
      return Traits::make_constant(*lhs_val * *rhs_val);
    }

    // Identity: 1 * expr → expr
    if (lhs_val && *lhs_val == scalar_number{1}) {
      return m_rhs;
    }
    if (rhs_val && *rhs_val == scalar_number{1}) {
      return m_lhs;
    }

    // Create mul n_ary tree: numerics as coeff, others as children
    auto mul_new{make_expression<mul_type>()};
    auto &mul{mul_new.template get<mul_type>()};
    if (lhs_val) {
      mul.set_coeff(m_lhs);
    } else {
      mul.push_back(m_lhs);
    }
    if (rhs_val) {
      mul.set_coeff(m_rhs);
    } else {
      mul.push_back(m_rhs);
    }
    return mul_new;
  }

  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

  // expr * 0 → 0
  expr_holder_t dispatch(typename Traits::zero_type const &) { return m_rhs; }

  // expr * 1 → expr
  expr_holder_t dispatch(typename Traits::one_type const &) { return m_lhs; }

  // x * (x*y*z) → push x into existing mul
  expr_holder_t dispatch(typename Traits::mul_type const &rhs) {
    auto mul{make_expression<typename Traits::mul_type>(rhs)};
    mul.template get<typename Traits::mul_type>().push_back(m_lhs);
    return mul;
  }

protected:
  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

} // namespace detail
} // namespace numsim::cas

#endif // SIMPLIFIER_MUL_H
