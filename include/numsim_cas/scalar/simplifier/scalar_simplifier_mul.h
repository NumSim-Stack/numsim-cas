#ifndef SCALAR_SIMPLIFIER_MUL_H
#define SCALAR_SIMPLIFIER_MUL_H

#include <numsim_cas/core/operators.h>
#include <numsim_cas/scalar/scalar_definitions.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_functions.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>

namespace numsim::cas {

namespace simplifier {
template <typename ExprLHS, typename ExprRHS>
class mul_default : public scalar_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<scalar_expression>;

  mul_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  template <typename Expr> inline expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

  // expr_lhs * (-expr_rhs) -->  -(expr_lhs * expr_rhs)
  inline expr_holder_t dispatch(scalar_negative const &rhs) {
    return -(std::forward<ExprLHS>(m_lhs) * rhs.expr());
  }

  // expr * zero --> zero
  inline expr_holder_t dispatch(scalar_zero const &) { return m_rhs; }

  // expr * 1 --> expr
  inline expr_holder_t dispatch(scalar_one const &) { return m_lhs; }

  // x * (x*y*z) --> 2*x*y*z
  inline expr_holder_t dispatch(scalar_mul const &rhs) {
    auto mul{make_expression<scalar_mul>(rhs)};
    mul.get<scalar_mul>().push_back(m_lhs);
    return mul;
  }

protected:
  expr_holder_t get_default() {
    if (m_lhs == m_rhs) {
      return pow(m_lhs, 2);
    }
    const auto lhs_constant{is_same<scalar_constant>(m_lhs)};
    const auto rhs_constant{is_same<scalar_constant>(m_rhs)};
    if (lhs_constant && m_lhs.template get<scalar_constant>().value() == 1) {
      return std::forward<ExprRHS>(m_rhs);
    }
    if (rhs_constant && m_rhs.template get<scalar_constant>().value() == 1) {
      return std::forward<ExprLHS>(m_lhs);
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

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class constant_mul final : public mul_default<ExprLHS, ExprRHS> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = mul_default<ExprLHS, ExprRHS>;
  using base::dispatch;
  using base::get_coefficient;
  using base::m_rhs;

  constant_mul(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_constant>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  inline expr_holder_t dispatch(scalar_constant const &rhs) {
    const auto value{lhs.value() * rhs.value()};
    return make_expression<scalar_constant>(value);
  }

  inline expr_holder_t dispatch([[maybe_unused]] scalar_mul const &rhs) {
    if (lhs.value() == 1) {
      return std::forward<ExprRHS>(m_rhs);
    }
    auto mul_expr{make_expression<scalar_mul>(rhs)};
    auto &mul{mul_expr.template get<scalar_mul>()};
    auto coeff{get_coefficient(mul, 1) * lhs.value()};
    mul.set_coeff(make_expression<scalar_constant>(coeff));
    return mul_expr;
  }

  template <typename ExprType>
  inline expr_holder_t operator()([[maybe_unused]] ExprType const &rhs) {
    if (lhs.value() == 1) {
      return std::forward<ExprRHS>(m_rhs);
    }
    return base::get_default();
  }

private:
  scalar_constant const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class n_ary_mul final : public mul_default<ExprLHS, ExprRHS> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = mul_default<ExprLHS, ExprRHS>;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  n_ary_mul(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_mul>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  // expr * constant
  inline expr_holder_t dispatch([[maybe_unused]] scalar_constant const &rhs) {
    auto mul_expr{make_expression<scalar_mul>(lhs)};
    auto &mul{mul_expr.template get<scalar_mul>()};
    auto coeff{mul.coeff().is_valid() ? mul.coeff() * m_rhs : m_rhs};
    mul.set_coeff(std::move(coeff));
    return mul_expr;
  }

  auto dispatch([[maybe_unused]] scalar const &rhs) {
    /// do a deep copy of data
    auto expr_mul{make_expression<scalar_mul>(lhs)};
    auto &mul{expr_mul.template get<scalar_mul>()};
    /// check if sub_exp == expr_rhs for sub_exp \in expr_lhs
    auto pos{lhs.hash_map().find(m_rhs)};
    if (pos != lhs.hash_map().end()) {
      auto expr{binary_scalar_mul_simplify(pos->second, m_rhs)};
      mul.hash_map().erase(m_rhs);
      mul.push_back(expr);
      return expr_mul;
    }

    const auto pows{get_all<scalar_pow>(lhs)};
    for (const auto &expr : pows) {
      if (expr.get<scalar_pow>().expr_lhs() == m_rhs) {
        mul.hash_map().erase(expr);
        expr_mul = std::move(expr_mul) * (expr * m_rhs);
        return expr_mul;
      }
    }

    /// no equal expr or sub_expr
    mul.push_back(m_rhs);
    return expr_mul;
  }

  auto dispatch([[maybe_unused]] scalar_pow const &rhs) {
    auto expr_mul{make_expression<scalar_mul>(lhs)};
    auto &mul{expr_mul.template get<scalar_mul>()};

    const auto &hash_map{lhs.hash_map()};
    if (hash_map.contains(rhs.expr_lhs())) {
      mul.hash_map().erase(rhs.expr_lhs());
      expr_mul = std::move(expr_mul) *
                 pow(rhs.expr_lhs(), rhs.expr_rhs() + get_scalar_one());
      return expr_mul;
    }

    const auto pows{get_all<scalar_pow>(lhs)};
    for (const auto &expr : pows) {
      if (auto pow{simplify_scalar_pow_pow_mul(expr.template get<scalar_pow>(),
                                               rhs)}) {
        mul.hash_map().erase(expr);
        expr_mul = std::move(expr_mul) * std::move(*pow);
        return expr_mul;
      }
    }

    mul.push_back(std::forward<ExprRHS>(m_rhs));
    return expr_mul;
  }

  // (x*y*z)*(x*y*z)
  auto dispatch([[maybe_unused]] scalar_mul const &rhs) {
    auto expr_mul{make_expression<scalar_mul>(lhs)};

    if (rhs.coeff().is_valid())
      expr_mul *= rhs.coeff();

    for (const auto &expr : rhs.hash_map() | std::views::values) {
      expr_mul = expr_mul * expr;
    }
    return expr_mul;
  }

  template <typename Expr> auto dispatch([[maybe_unused]] Expr const &rhs) {
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

template <typename ExprLHS, typename ExprRHS>
class scalar_pow_mul final : public mul_default<ExprLHS, ExprRHS> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = mul_default<ExprLHS, ExprRHS>;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  scalar_pow_mul(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_pow>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  auto dispatch([[maybe_unused]] scalar const &rhs) {
    const auto &power{lhs.expr_rhs()};
    const auto &base{lhs.expr_lhs()};
    if (base == m_rhs) {
      const auto rhs_expr{lhs.expr_rhs() + get_scalar_one()};
      return pow(lhs.expr_lhs(), std::move(rhs_expr));
    }

    // pow(expr, -expr_p) * expr_p --> expr
    if (is_same<scalar_negative>(power) &&
        power.get<scalar_negative>().expr() == m_rhs) {
      return lhs.expr_lhs();
    }
    return get_default();
  }

  // pow(expr, base_lhs) * pow(expr, base_rhs) --> pow(expr, base_lhs+base_rhs)
  // pow(expr_lhs, base) * pow(expr_rhs, base) --> pow(expr_lhs * expr_rhs,
  // base)
  auto dispatch([[maybe_unused]] scalar_pow const &rhs) {
    if (lhs.expr_lhs() == rhs.expr_lhs()) {
      const auto rhs_expr{lhs.expr_rhs() + rhs.expr_rhs()};
      return pow(lhs.expr_lhs(), std::move(rhs_expr));
    }

    if (lhs.expr_rhs() == rhs.expr_rhs()) {
      const auto lhs_expr{lhs.expr_lhs() * rhs.expr_lhs()};
      return pow(std::move(lhs_expr), lhs.expr_rhs());
    }

    return get_default();
  }

  // pow(x,1) * (x*y*z)
  auto dispatch([[maybe_unused]] scalar_mul const &rhs) {
    auto expr_mul{make_expression<scalar_mul>(rhs)};
    auto &mul{expr_mul.template get<scalar_mul>()};

    const auto &hash_map{rhs.hash_map()};
    if (hash_map.contains(lhs.expr_lhs())) {
      mul.hash_map().erase(lhs.expr_lhs());
      expr_mul = std::move(expr_mul) *
                 pow(lhs.expr_lhs(), lhs.expr_rhs() + get_scalar_one());
      return expr_mul;
    }

    const auto pows{get_all<scalar_pow>(rhs)};
    for (const auto &expr : pows) {
      if (auto pow{simplify_scalar_pow_pow_mul(
              lhs, expr.template get<scalar_pow>())}) {
        mul.hash_map().erase(expr);
        expr_mul = std::move(expr_mul) * std::move(*pow);
        return expr_mul;
      }
    }

    mul.push_back(std::forward<ExprLHS>(m_lhs));
    return expr_mul;
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_pow const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class symbol_mul final : public mul_default<ExprLHS, ExprRHS> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = mul_default<ExprLHS, ExprRHS>;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  symbol_mul(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  /// x*x --> pow(x,2)
  inline expr_holder_t dispatch(scalar const &rhs) {
    if (&lhs == &rhs) {
      return make_expression<scalar_pow>(std::forward<ExprRHS>(m_rhs),
                                         make_expression<scalar_constant>(2));
    }
    return get_default();
  }

  auto dispatch([[maybe_unused]] scalar_mul const &rhs) {
    /// do a deep copy of data
    auto expr_mul{make_expression<scalar_mul>(rhs)};
    auto &mul{expr_mul.template get<scalar_mul>()};
    /// check if sub_exp == expr_rhs for sub_exp \in expr_lhs
    auto pos{rhs.hash_map().find(m_lhs)};
    if (pos != rhs.hash_map().end()) {
      // auto expr{binary_scalar_mul_simplify(pos->second, m_lhs)};
      auto expr{pos->second * m_lhs};
      mul.hash_map().erase(m_lhs);
      mul.push_back(expr);
      return expr_mul;
    }
    /// no equal expr or sub_expr
    mul.push_back(m_lhs);
    return expr_mul;
  }

  /// x * pow(x,expr) --> pow(x,expr+1)
  inline expr_holder_t dispatch(scalar_pow const &rhs) {
    if (m_lhs == rhs.expr_lhs()) {
      return pow(m_lhs, rhs.expr_rhs() + get_scalar_one());
    }

    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
struct mul_base final : public scalar_visitor_return_expr_t {
  using expr_holder_t = expression_holder<scalar_expression>;

  mul_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  inline expr_holder_t dispatch(scalar_negative const &lhs) {
    auto expr{lhs.expr() * m_rhs};
    return -expr;
  }

  inline expr_holder_t dispatch(scalar_constant const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    constant_mul<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                           std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  inline expr_holder_t dispatch(scalar_mul const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    n_ary_mul<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                        std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  inline expr_holder_t dispatch(scalar const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    symbol_mul<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                         std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  inline expr_holder_t dispatch(scalar_pow const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    scalar_pow_mul<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                             std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  // zero * expr --> zero
  inline expr_holder_t dispatch(scalar_zero const &) { return m_lhs; }

  // one * expr --> expr
  inline expr_holder_t dispatch(scalar_one const &) { return m_rhs; }

  template <typename Type> inline expr_holder_t dispatch(Type const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    mul_default<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                          std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

} // namespace simplifier
} // namespace numsim::cas
#endif // SCALAR_SIMPLIFIER_MUL_H
