#ifndef SCALAR_SIMPLIFIER_ADD_H
#define SCALAR_SIMPLIFIER_ADD_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/scalar_number.h>
#include <numsim_cas/functions.h>
#include <numsim_cas/scalar/scalar_definitions.h>
#include <numsim_cas/scalar/scalar_functions.h>

namespace numsim::cas {
namespace simplifier {

template <typename Derived>
class add_default : public scalar_visitor_return_expr_t {
public:
  using expr_t = scalar_expression;
  using expr_holder_t = expression_holder<expr_t>;

  add_default(expr_holder_t lhs, expr_holder_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

protected:
  auto get_default() {
    const auto lhs_constant{is_constant(m_lhs)};
    const auto rhs_constant{is_constant(m_rhs)};

    assert(!(lhs_constant && rhs_constant));

    auto add_new{make_expression<scalar_add>()};
    if (lhs_constant) {
      auto &add{add_new.template get<scalar_add>()};
      add.set_coeff(m_lhs);
    } else {
      add_new.template get<scalar_add>().push_back(m_lhs);
    }

    if (rhs_constant) {
      auto &add{add_new.template get<scalar_add>()};
      add.set_coeff(m_rhs);
    } else {
      add_new.template get<scalar_add>().push_back(m_rhs);
    }
    return add_new;
  }

  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

  expr_holder_t dispatch(scalar_zero const &) { return m_lhs; }

  expr_holder_t dispatch(scalar_negative const &expr) {
    if (m_lhs == expr.expr()) {
      return get_scalar_zero();
    }
    return get_default();
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

#define NUMSIM_ADD_OVR(T)                                                      \
  expr_holder_t operator()(T const &n) override {                              \
    if constexpr (std::is_void_v<Derived>) {                                   \
      return dispatch(n);                                                      \
    } else {                                                                   \
      return static_cast<Derived *>(this)->dispatch(n);                        \
    }                                                                          \
  }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR, NUMSIM_ADD_OVR)
#undef NUMSIM_ADD_OVR

protected:
  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

class constant_add final : public add_default<constant_add> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = add_default<constant_add>;
  using base::operator();
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  constant_add(expr_holder_t lhs, expr_holder_t rhs);

  expr_holder_t dispatch(scalar_constant const &rhs);

  expr_holder_t dispatch([[maybe_unused]] scalar_add const &rhs);

  expr_holder_t dispatch(scalar_one const &);

  expr_holder_t dispatch(scalar_negative const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_constant const &lhs;
};

class add_scalar_one final : public add_default<add_scalar_one> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = add_default<add_scalar_one>;
  using base::operator();
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  add_scalar_one(expr_holder_t lhs, expr_holder_t rhs);

  expr_holder_t dispatch(scalar_constant const &rhs);

  expr_holder_t dispatch([[maybe_unused]] scalar_add const &rhs);

  expr_holder_t dispatch(scalar_one const &);

  expr_holder_t dispatch(scalar_negative const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_one const &lhs;
};

class n_ary_add final : public add_default<n_ary_add> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = add_default<n_ary_add>;
  using base::operator();
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  n_ary_add(expr_holder_t lhs, expr_holder_t rhs);

  expr_holder_t dispatch([[maybe_unused]] scalar_constant const &rhs);

  expr_holder_t dispatch([[maybe_unused]] scalar_one const &);

  expr_holder_t dispatch([[maybe_unused]] scalar const &rhs);

  // merge two expression
  expr_holder_t dispatch(scalar_add const &rhs);

  expr_holder_t dispatch(scalar_negative const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_add const &lhs;
};

class n_ary_mul_add final : public add_default<n_ary_mul_add> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;

  using base = add_default<n_ary_mul_add>;
  using base::operator();
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  n_ary_mul_add(expr_holder_t lhs, expr_holder_t rhs);

  // constant*expr + expr --> (constant+1)*expr
  expr_holder_t dispatch([[maybe_unused]] scalar const &rhs);

  /// expr + expr --> 2*expr
  expr_holder_t dispatch(scalar_mul const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_mul const &lhs;
};

class symbol_add final : public add_default<symbol_add> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = add_default<symbol_add>;
  using base::operator();
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  symbol_add(expr_holder_t lhs, expr_holder_t rhs);

  /// x+x --> 2*x
  expr_holder_t dispatch(scalar const &rhs);

  expr_holder_t dispatch(scalar_mul const &rhs);
  //   expr_holder_t operator()(scalar_constant
  //  const&rhs) {
  //  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar const &lhs;
};

class add_negative final : public add_default<add_negative> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = add_default<add_negative>;
  using base::operator();
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  add_negative(expr_holder_t lhs, expr_holder_t rhs);

  // (-lhs) + (-rhs) --> -(lhs+rhs)
  expr_holder_t dispatch(scalar_negative const &rhs);

  expr_holder_t dispatch([[maybe_unused]] scalar_add const &rhs);

  // -expr + c
  expr_holder_t dispatch(scalar_constant const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_negative const &lhs;
};

struct add_base final : public scalar_visitor_return_expr_t {
  using expr_t = scalar_expression;
  using expr_holder_t = expression_holder<scalar_expression>;

  add_base(expr_holder_t lhs, expr_holder_t rhs);

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

private:
  expr_holder_t dispatch(scalar_constant const &);

  expr_holder_t dispatch(scalar_one const &);

  expr_holder_t dispatch(scalar_add const &);

  expr_holder_t dispatch(scalar_mul const &);

  expr_holder_t dispatch(scalar const &);

  expr_holder_t dispatch(scalar_negative const &);

  expr_holder_t dispatch(scalar_zero const &);

  template <typename Type> expr_holder_t dispatch(Type const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    add_default<void> visitor(std::move(m_lhs), std::move(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};
} // namespace simplifier

} // namespace numsim::cas
#endif // SCALAR_SIMPLIFIER_ADD_H
