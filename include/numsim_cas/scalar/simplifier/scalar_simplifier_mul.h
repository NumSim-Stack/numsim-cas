#ifndef SCALAR_SIMPLIFIER_MUL_H
#define SCALAR_SIMPLIFIER_MUL_H

#include <numsim_cas/core/simplifier/simplifier_mul.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_definitions.h>
#include <numsim_cas/scalar/scalar_domain_traits.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_functions.h>
#include <numsim_cas/scalar/scalar_std.h>

namespace numsim::cas {
namespace simplifier {

using scalar_traits = domain_traits<scalar_expression>;

template <typename Derived>
class mul_default : public scalar_visitor_return_expr_t,
                    public detail::mul_dispatch<scalar_traits, Derived> {
  using algo = detail::mul_dispatch<scalar_traits, Derived>;

public:
  using expr_holder_t = typename algo::expr_holder_t;
  using algo::algo;
  using algo::dispatch;
  using algo::get_default;
  // expr_lhs * (-expr_rhs) -->  -(expr_lhs * expr_rhs)
  expr_holder_t dispatch(scalar_negative const &rhs) {
    return -(std::move(m_lhs) * rhs.expr());
  }

protected:
#define NUMSIM_MUL_OVR(T)                                                      \
  expr_holder_t operator()(T const &n) override {                              \
    if constexpr (std::is_void_v<Derived>) {                                   \
      return dispatch(n);                                                      \
    } else {                                                                   \
      return static_cast<Derived *>(this)->dispatch(n);                        \
    }                                                                          \
  }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_MUL_OVR, NUMSIM_MUL_OVR)
#undef NUMSIM_MUL_OVR

  using algo::m_lhs;
  using algo::m_rhs;
};

class constant_mul final : public mul_default<constant_mul> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = mul_default<constant_mul>;
  using base::operator();
  using base::dispatch;

  using base::m_rhs;

  constant_mul(expr_holder_t lhs, expr_holder_t rhs);

  expr_holder_t dispatch(scalar_constant const &rhs);

  expr_holder_t dispatch([[maybe_unused]] scalar_mul const &rhs);

  template <typename ExprType>
  expr_holder_t operator()([[maybe_unused]] ExprType const &rhs) {
    if (m_lhs_node.value() == 1) {
      return std::move(m_rhs);
    }
    return base::get_default();
  }

private:
  scalar_constant const &m_lhs_node;
};

class n_ary_mul final : public mul_default<n_ary_mul> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = mul_default<n_ary_mul>;
  using base::operator();
  using base::dispatch;

  using base::get_default;

  n_ary_mul(expr_holder_t lhs, expr_holder_t rhs);

  // expr * constant
  expr_holder_t dispatch([[maybe_unused]] scalar_constant const &rhs);

  expr_holder_t dispatch([[maybe_unused]] scalar const &rhs);

  expr_holder_t dispatch([[maybe_unused]] scalar_pow const &rhs);

  // (x*y*z)*(x*y*z)
  expr_holder_t dispatch([[maybe_unused]] scalar_mul const &rhs);

  // (x*y*z)*exp(a) --> scan for exp child, combine
  expr_holder_t dispatch([[maybe_unused]] scalar_exp const &rhs);

  template <typename Expr>
  expr_holder_t dispatch([[maybe_unused]] Expr const &rhs) {
    auto expr_mul{make_expression<scalar_mul>(m_lhs_node)};
    auto &mul{expr_mul.template get<scalar_mul>()};
    mul.push_back(m_rhs);
    return expr_mul;
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_mul const &m_lhs_node;
};

class exp_mul final : public mul_default<exp_mul> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = mul_default<exp_mul>;
  using base::operator();
  using base::dispatch;

  using base::get_default;

  exp_mul(expr_holder_t lhs, expr_holder_t rhs);

  // exp(a)*exp(b) --> exp(a+b)
  expr_holder_t dispatch(scalar_exp const &rhs);

  // exp(a)*(x*exp(b)*...) --> x*exp(a+b)*...
  expr_holder_t dispatch(scalar_mul const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_exp const &m_lhs_node;
};

class scalar_pow_mul final : public mul_default<scalar_pow_mul> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = mul_default<scalar_pow_mul>;
  using base::operator();
  using base::dispatch;

  using base::get_default;

  scalar_pow_mul(expr_holder_t lhs, expr_holder_t rhs);

  expr_holder_t dispatch([[maybe_unused]] scalar const &rhs);

  // pow(expr, base_lhs) * pow(expr, base_rhs) --> pow(expr, base_lhs+base_rhs)
  // pow(expr_lhs, base) * pow(expr_rhs, base) --> pow(expr_lhs * expr_rhs,
  // base)
  expr_holder_t dispatch([[maybe_unused]] scalar_pow const &rhs);

  // pow(x,1) * (x*y*z)
  expr_holder_t dispatch([[maybe_unused]] scalar_mul const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_pow const &m_lhs_node;
};

class symbol_mul final : public mul_default<symbol_mul> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = mul_default<symbol_mul>;
  using base::operator();
  using base::dispatch;

  using base::get_default;

  symbol_mul(expr_holder_t lhs, expr_holder_t rhs);

  /// x*x --> pow(x,2)
  expr_holder_t dispatch(scalar const &rhs);

  expr_holder_t dispatch([[maybe_unused]] scalar_mul const &rhs);

  /// x * pow(x,expr) --> pow(x,expr+1)
  expr_holder_t dispatch(scalar_pow const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar const &m_lhs_node;
};

struct mul_base final : public scalar_visitor_return_expr_t {
  using expr_holder_t = expression_holder<scalar_expression>;

  mul_base(expr_holder_t lhs, expr_holder_t rhs);

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  expr_holder_t dispatch(scalar_negative const &lhs);

  expr_holder_t dispatch(scalar_constant const &);

  expr_holder_t dispatch(scalar_mul const &);

  expr_holder_t dispatch(scalar const &);

  expr_holder_t dispatch(scalar_pow const &);

  expr_holder_t dispatch(scalar_exp const &);

  // zero * expr --> zero
  expr_holder_t dispatch(scalar_zero const &);

  // one * expr --> expr
  expr_holder_t dispatch(scalar_one const &);

  template <typename Type> expr_holder_t dispatch(Type const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    mul_default<void> visitor(std::move(m_lhs), std::move(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

} // namespace simplifier
} // namespace numsim::cas
#endif // SCALAR_SIMPLIFIER_MUL_H
