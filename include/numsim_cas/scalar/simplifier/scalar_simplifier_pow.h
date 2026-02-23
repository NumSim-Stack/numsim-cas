#ifndef SCALAR_SIMPLIFIER_POW_H
#define SCALAR_SIMPLIFIER_POW_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/core/simplifier/simplifier_pow.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_definitions.h>
#include <numsim_cas/scalar/scalar_domain_traits.h>
#include <numsim_cas/scalar/scalar_globals.h>
#include <numsim_cas/scalar/scalar_visitor_typedef.h>

namespace numsim::cas::simplifier {

using scalar_traits = domain_traits<scalar_expression>;

template <typename Derived>
class pow_default : public scalar_visitor_return_expr_t,
                    public detail::pow_dispatch<scalar_traits, Derived> {
  using algo = detail::pow_dispatch<scalar_traits, Derived>;

public:
  using expr_t = scalar_expression;
  using expr_holder_t = expression_holder<expr_t>;

  pow_default(expr_holder_t lhs, expr_holder_t rhs)
      : algo(std::move(lhs), std::move(rhs)) {}

protected:
#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override {                              \
    if constexpr (std::is_void_v<Derived>) {                                   \
      return this->dispatch(n);                                                \
    } else {                                                                   \
      return static_cast<Derived *>(this)->dispatch(n);                        \
    }                                                                          \
  }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER
};

class pow_pow final : public pow_default<pow_pow> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = pow_default<pow_pow>;
  using base::operator();
  using base::dispatch;
  using base::get_default;

  pow_pow(expr_holder_t lhs, expr_holder_t rhs);

  /// pow(pow(x,a),b) --> pow(x,a*b)
  /// TODO? pow(pow(x,-a), -b) --> pow(x,-a*b) only when x,a,b>0
  template <typename Expr> expr_holder_t dispatch(Expr const &);

  expr_holder_t dispatch(scalar_negative const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_pow const &m_lhs_node;
};

class mul_pow final : public pow_default<mul_pow> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = pow_default<mul_pow>;
  using base::operator();
  using base::dispatch;
  using base::get_default;

  mul_pow(expr_holder_t lhs, expr_holder_t rhs);

  // pow(scalar_mul, -rhs)
  expr_holder_t dispatch(scalar_negative const &rhs);

  template <typename Expr>
  expr_holder_t dispatch([[maybe_unused]] Expr const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_mul const &m_lhs_node;
};

struct pow_base final : public scalar_visitor_return_expr_t {
  using expr_holder_t = expression_holder<scalar_expression>;

  pow_base(expr_holder_t lhs, expr_holder_t rhs);

protected:
#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  expr_holder_t dispatch(scalar_pow const &);

  expr_holder_t dispatch(scalar_mul const &);

  expr_holder_t dispatch(scalar_exp const &);

  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    pow_default<void> visitor(std::move(m_lhs), std::move(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

} // namespace numsim::cas::simplifier

#endif // SCALAR_SIMPLIFIER_POW_H
