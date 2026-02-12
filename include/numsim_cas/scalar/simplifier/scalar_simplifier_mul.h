#ifndef SCALAR_SIMPLIFIER_MUL_H
#define SCALAR_SIMPLIFIER_MUL_H

#include <numsim_cas/scalar/scalar_definitions.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_functions.h>
#include <numsim_cas/scalar/scalar_std.h>

namespace numsim::cas {
namespace simplifier {

template <typename Derived>
class mul_default : public scalar_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<scalar_expression>;

  mul_default(expr_holder_t lhs, expr_holder_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

  // expr_lhs * (-expr_rhs) -->  -(expr_lhs * expr_rhs)
  expr_holder_t dispatch(scalar_negative const &rhs);

  // expr * zero --> zero
  expr_holder_t dispatch(scalar_zero const &) { return m_rhs; }

  // expr * 1 --> expr
  expr_holder_t dispatch(scalar_one const &) { return m_lhs; }

  // x * (x*y*z) --> 2*x*y*z
  expr_holder_t dispatch(scalar_mul const &rhs) {
    auto mul{make_expression<scalar_mul>(rhs)};
    mul.get<scalar_mul>().push_back(m_lhs);
    return mul;
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

  expr_holder_t get_default() {
    if (m_lhs == m_rhs) {
      return pow(m_lhs, 2);
    }
    const auto lhs_constant{is_same<scalar_constant>(m_lhs)};
    const auto rhs_constant{is_same<scalar_constant>(m_rhs)};
    if (lhs_constant && m_lhs.template get<scalar_constant>().value() == 1) {
      return std::move(m_rhs);
    }
    if (rhs_constant && m_rhs.template get<scalar_constant>().value() == 1) {
      return std::move(m_lhs);
    }
    auto mul_new{make_expression<scalar_mul>()};
    auto &mul{mul_new.template get<scalar_mul>()};
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
    return mul_new;
  }

  template <typename _Expr, typename _ValueType>
  scalar_number get_coefficient(_Expr const &expr, _ValueType const &value) {
    if (is_detected_v<has_coefficient, _Expr>) {
      auto func{[&](auto const &coeff) -> scalar_number {
        if (coeff.is_valid()) {
          if (is_same<scalar_negative>(coeff)) {
            const auto &neg_expr{coeff.template get<scalar_negative>().expr()};
            if (is_same<scalar_one>(neg_expr))
              return {-1};
            if (is_same<scalar_constant>(neg_expr))
              return -neg_expr.template get<scalar_constant>().value();
          } else {
            if (is_same<scalar_one>(coeff))
              return {1};
            if (is_same<scalar_constant>(coeff))
              return coeff.template get<scalar_constant>().value();
          }
        }

        return value;
      }};
      return func(expr.coeff());
    }
    return value;
  }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

class constant_mul final : public mul_default<constant_mul> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = mul_default<constant_mul>;
  using base::dispatch;
  using base::get_coefficient;
  using base::m_rhs;

  constant_mul(expr_holder_t lhs, expr_holder_t rhs);

  expr_holder_t dispatch(scalar_constant const &rhs);

  expr_holder_t dispatch([[maybe_unused]] scalar_mul const &rhs);

  template <typename ExprType>
  expr_holder_t operator()([[maybe_unused]] ExprType const &rhs) {
    if (lhs.value() == 1) {
      return std::move(m_rhs);
    }
    return base::get_default();
  }

private:
  scalar_constant const &lhs;
};

class n_ary_mul final : public mul_default<n_ary_mul> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = mul_default<n_ary_mul>;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  n_ary_mul(expr_holder_t lhs, expr_holder_t rhs);

  // expr * constant
  expr_holder_t dispatch([[maybe_unused]] scalar_constant const &rhs);

  expr_holder_t dispatch([[maybe_unused]] scalar const &rhs);

  expr_holder_t dispatch([[maybe_unused]] scalar_pow const &rhs);

  // (x*y*z)*(x*y*z)
  expr_holder_t dispatch([[maybe_unused]] scalar_mul const &rhs);

  template <typename Expr>
  expr_holder_t dispatch([[maybe_unused]] Expr const &rhs) {
    auto expr_mul{make_expression<scalar_mul>(lhs)};
    auto &mul{expr_mul.template get<scalar_mul>()};
    mul.push_back(m_rhs);
    return expr_mul;
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_mul const &lhs;
};

class scalar_pow_mul final : public mul_default<scalar_pow_mul> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = mul_default<scalar_pow_mul>;
  using base::dispatch;
  using base::get_coefficient;
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
  scalar_pow const &lhs;
};

class symbol_mul final : public mul_default<symbol_mul> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = mul_default<symbol_mul>;
  using base::dispatch;
  using base::get_coefficient;
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
  scalar const &lhs;
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
