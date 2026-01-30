#ifndef SCALAR_SIMPLIFIER_POW_H
#define SCALAR_SIMPLIFIER_POW_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/scalar/scalar_definitions.h>
#include <numsim_cas/scalar/scalar_globals.h>
#include <numsim_cas/scalar/scalar_visitor_typedef.h>

namespace numsim::cas::simplifier {

template <typename ExprLHS, typename ExprRHS>
class pow_default : public scalar_visitor_return_expr_t {
public:
  using expr_t = scalar_expression;
  using expr_holder_t = expression_holder<expr_t>;

  pow_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

protected:
  auto get_default() {
    if (is_same<scalar_negative>(m_rhs)) {
      const auto expr{m_rhs.get<scalar_negative>().expr()};
      // expr / expr --> 1; for x /= 0
      if (m_lhs == expr) {
        return get_scalar_one();
      }

      // expr*x / x --> expr; for x /= 0
      if (is_same<scalar_mul>(m_lhs)) {
        const auto &map{m_lhs.get<scalar_mul>().hash_map()};
        auto pos{map.find(expr)};
        if (pos != map.end()) {
          auto copy{make_expression<scalar_mul>(m_lhs.get<scalar_mul>())};
          copy.get<scalar_mul>().hash_map().erase(expr);
          return copy;
        }
      }
    }
    return make_expression<scalar_pow>(std::forward<ExprLHS>(m_lhs),
                                       std::forward<ExprRHS>(m_rhs));
  }

  template <typename Expr> inline expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

protected:
  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class pow_pow final : public pow_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<scalar_expression>;
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = pow_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::dispatch;
  using base::get_default;

  pow_pow(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_pow>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_type operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_type operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  /// pow(pow(x,a),b) --> pow(x,a*b)
  /// TODO? pow(pow(x,-a), -b) --> pow(x,-a*b) only when x,a,b>0
  template <typename Expr> inline expr_type dispatch(Expr const &) {
    return pow(lhs.expr_lhs(), lhs.expr_rhs() * m_rhs);
  }

  inline expr_type dispatch(scalar_negative const &rhs) {
    if (lhs.expr_lhs() == rhs.expr() &&
        is_same<scalar_constant>(lhs.expr_rhs())) {
      return pow(lhs.expr_lhs(), lhs.expr_rhs() - 1);
    }
    return pow(lhs.expr_lhs(), lhs.expr_rhs() * m_rhs);
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_pow const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class mul_pow final : public pow_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<scalar_expression>;
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = pow_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::dispatch;
  using base::get_default;

  mul_pow(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_mul>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_type operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_type operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  // pow(scalar_mul, -rhs)
  inline expr_type dispatch(scalar_negative const &rhs) {
    auto pos{lhs.hash_map().find(rhs.expr())};
    auto mul_expr{make_expression<scalar_mul>(lhs)};
    auto &mul{mul_expr.template get<scalar_mul>()};
    // x*y*z / x --> y*z
    if (pos != lhs.hash_map().end()) {
      mul.hash_map().erase(rhs.expr());
      return mul_expr;
    }

    // pow(x*y*pow(z,base), rhs) --> pow(x*y, rhs) * pos(z,base*rhs)
    const auto pows{get_all<scalar_pow>(lhs)};
    if (!pows.empty()) {
      expr_holder_t result;
      for (const auto &expr : pows) {
        const auto &pow_expr{expr.get<scalar_pow>()};
        mul.hash_map().erase(expr);
        auto pow_n{pow(pow_expr.expr_lhs(), pow_expr.expr_rhs() * m_rhs)};
        if (!result.is_valid()) {
          result = std::move(pow_n);
        } else {
          result = result * std::move(pow_n);
        }
      }
      return pow(mul_expr, m_rhs) * result;
    }

    return get_default();
  }

  template <typename Expr>
  inline expr_type dispatch([[maybe_unused]] Expr const &rhs) {
    auto mul_expr{make_expression<scalar_mul>(lhs)};
    auto &mul{mul_expr.template get<scalar_mul>()};

    // pow(x*y*pow(z,base), rhs) --> pow(x*y, rhs) * pos(z,base*rhs)
    const auto pows{get_all<scalar_pow>(lhs)};
    if (!pows.empty()) {
      expr_holder_t result;
      for (const auto &expr : pows) {
        const auto &pow_expr{expr.get<scalar_pow>()};
        mul.hash_map().erase(expr);
        auto pow_n{pow(pow_expr.expr_lhs(), pow_expr.expr_rhs() * m_rhs)};
        if (!result.is_valid()) {
          result = std::move(pow_n);
        } else {
          result = result * std::move(pow_n);
        }
      }
      return pow(mul_expr, m_rhs) * result;
    }

    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_mul const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
struct pow_base final : public scalar_visitor_return_expr_t {
  using expr_t = scalar_expression;
  using expr_holder_t = expression_holder<scalar_expression>;

  pow_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

private:
  expr_holder_t dispatch(scalar_pow const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    pow_pow<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                      std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t dispatch(scalar_mul const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    mul_pow<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                      std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    pow_default<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                          std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

} // namespace numsim::cas::simplifier

#endif // SCALAR_SIMPLIFIER_POW_H
