#ifndef SCALAR_SIMPLIFIER_POW_H
#define SCALAR_SIMPLIFIER_POW_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/scalar/scalar_definitions.h>
#include <numsim_cas/scalar/scalar_globals.h>
#include <numsim_cas/scalar/scalar_visitor_typedef.h>

namespace numsim::cas::simplifier {

template <typename Derived>
class pow_default : public scalar_visitor_return_expr_t {
public:
  using expr_t = scalar_expression;
  using expr_holder_t = expression_holder<expr_t>;

  pow_default(expr_holder_t lhs, expr_holder_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

protected:
#define NUMSIM_LOOP_OVER(T)                                                    \
  expr_holder_t operator()(T const &n) override {                              \
    if constexpr (std::is_void_v<Derived>) {                                   \
      return dispatch(n);                                                      \
    } else {                                                                   \
      return static_cast<Derived *>(this)->dispatch(n);                        \
    }                                                                          \
  }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER

  expr_holder_t get_default() {
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
    // pow(-expr, power) --> -pow(expr, power)
    if (auto expr_neg{is_same_r<scalar_negative>(m_lhs)}) {
      return -make_expression<scalar_pow>(expr_neg->get().expr(),
                                          std::move(m_rhs));
    }
    return make_expression<scalar_pow>(std::move(m_lhs), std::move(m_rhs));
  }

  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

protected:
  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
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
  scalar_pow const &lhs;
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
  scalar_mul const &lhs;
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
