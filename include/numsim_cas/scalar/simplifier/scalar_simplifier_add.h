#ifndef SCALAR_SIMPLIFIER_ADD_H
#define SCALAR_SIMPLIFIER_ADD_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/core/scalar_number.h>
#include <numsim_cas/functions.h>
#include <numsim_cas/scalar/scalar_definitions.h>
#include <numsim_cas/scalar/scalar_functions.h>
#include <numsim_cas/scalar/scalar_operators.h>

namespace numsim::cas {
namespace simplifier {

template <typename ExprLHS, typename ExprRHS>
class add_default : public scalar_visitor_return_expr_t {
public:
  using expr_t = scalar_expression;
  using expr_holder_t = expression_holder<expr_t>;

  add_default(ExprLHS &&lhs, ExprRHS &&rhs)
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

  template <typename Expr> inline expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

  inline expr_holder_t dispatch(scalar_zero const &) { return m_lhs; }

  inline expr_holder_t dispatch(scalar_negative const &expr) {
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

protected:
  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class constant_add final : public add_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<scalar_expression>;
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  constant_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_constant>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  inline expr_type dispatch(scalar_constant const &rhs) {
    const auto value{lhs.value() + rhs.value()};
    return make_expression<scalar_constant>(value);
  }

  inline expr_type dispatch([[maybe_unused]] scalar_add const &rhs) {
    const auto value{get_coefficient(rhs, 0) + lhs.value()};
    if (value != 0) {
      auto add_expr{make_expression<scalar_add>(rhs)};
      auto &add{add_expr.template get<scalar_add>()};
      auto coeff{make_expression<scalar_constant>(value)};
      add.set_coeff(std::move(coeff));
      return add_expr;
    }
    return m_rhs;
  }

  inline expr_type dispatch(scalar_one const &) {
    const auto value{lhs.value() + 1};
    return make_expression<scalar_constant>(value);
  }

  inline expr_type dispatch(scalar_negative const &rhs) {
    if (is_same<scalar_one>(rhs.expr())) {
      scalar_number value = lhs.value() - 1;
      if (value == 0) {
        return get_scalar_zero();
      }
      return make_expression<scalar_constant>(value);
    }
    if (is_same<scalar_constant>(rhs.expr())) {
      scalar_number value =
          lhs.value() - rhs.expr().get<scalar_constant>().value();
      if (value == 0) {
        return get_scalar_zero();
      }
      return make_expression<scalar_constant>(value);
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_constant const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class add_scalar_one final : public add_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<scalar_expression>;
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  add_scalar_one(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_one>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  inline expr_type dispatch(scalar_constant const &rhs) {
    const auto value{1 + rhs.value()};
    if (value != 0)
      return make_expression<scalar_constant>(value);
    return get_scalar_zero();
  }

  inline expr_type dispatch([[maybe_unused]] scalar_add const &rhs) {
    const auto value{get_coefficient(rhs, 0) + 1};
    if (value != 0) {
      auto add_expr{make_expression<scalar_add>(rhs)};
      auto &add{add_expr.template get<scalar_add>()};
      auto coeff{make_expression<scalar_constant>(value)};
      add.set_coeff(std::move(coeff));
      return add_expr;
    }
    return m_rhs;
  }

  inline expr_type dispatch(scalar_one const &) {
    const auto value{1 + 1};
    return make_expression<scalar_constant>(value);
  }

  inline expr_type dispatch(scalar_negative const &rhs) {
    if (is_same<scalar_one>(rhs.expr())) {
      return get_scalar_zero();
    }
    if (is_same<scalar_constant>(rhs.expr())) {
      scalar_number value = 1 - rhs.expr().get<scalar_constant>().value();
      if (value == 0) {
        return get_scalar_zero();
      }
      return make_expression<scalar_constant>(value);
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_one const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class n_ary_add final : public add_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<scalar_expression>;
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  n_ary_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_add>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  inline expr_type dispatch([[maybe_unused]] scalar_constant const &rhs) {
    auto add_expr{make_expression<scalar_add>(lhs)};
    auto &add{add_expr.template get<scalar_add>()};
    const auto value{get_coefficient(lhs, 0) + rhs.value()};
    add.coeff().free();
    if (value != 0) {
      auto coeff{make_expression<scalar_constant>(value)};
      add.set_coeff(std::move(coeff));
      return add_expr;
    }
    return add_expr;
  }

  inline expr_type dispatch([[maybe_unused]] scalar_one const &) {
    auto add_expr{make_expression<scalar_add>(lhs)};
    auto &add{add_expr.template get<scalar_add>()};
    const auto value{get_coefficient(add, 0) + 1};
    add.coeff().free();
    if (value != 0) {
      auto coeff{make_expression<scalar_constant>(value)};
      add.set_coeff(std::move(coeff));
    }
    return add_expr;
  }

  auto dispatch([[maybe_unused]] scalar const &rhs) {
    /// do a deep copy of data
    auto expr_add{make_expression<scalar_add>(lhs)};
    auto &add{expr_add.template get<scalar_add>()};
    /// check if sub_exp == expr_rhs for sub_exp \in expr_lhs
    auto pos{add.hash_map().find(m_rhs)};
    if (pos != add.hash_map().end()) {
      auto expr{pos->second + m_rhs};
      add.hash_map().erase(pos);
      add.push_back(std::move(expr));
      return expr_add;
    }
    /// no equal expr or sub_expr
    add.push_back(m_rhs);
    return expr_add;
  }

  // merge two expression
  auto dispatch(scalar_add const &rhs) {
    auto expr{make_expression<scalar_add>()};
    auto &add{expr.template get<scalar_add>()};
    merge_add(lhs, rhs, add);
    return expr;
  }

  auto dispatch(scalar_negative const &rhs) {
    const auto pos{lhs.hash_map().find(rhs.expr())};
    if (pos != lhs.hash_map().end()) {
      auto expr{make_expression<scalar_add>(lhs)};
      auto &add{expr.template get<scalar_add>()};
      add.hash_map().erase(rhs.expr());
      return expr;
    }

    if (is_same<scalar_constant>(rhs.expr())) {
      auto add_expr{make_expression<scalar_add>(lhs)};
      auto &add{add_expr.template get<scalar_add>()};
      const auto value{get_coefficient(lhs, 0) -
                       rhs.expr().get<scalar_constant>().value()};
      add.coeff().free();
      if (value != 0) {
        auto coeff{make_expression<scalar_constant>(value)};
        add.set_coeff(std::move(coeff));
      }
      return add_expr;
    }

    if (is_same<scalar_one>(rhs.expr())) {
      auto add_expr{make_expression<scalar_add>(lhs)};
      auto &add{add_expr.template get<scalar_add>()};
      const auto value{get_coefficient(lhs, 0) - 1};
      add.coeff().free();
      if (value != 0) {
        auto coeff{make_expression<scalar_constant>(value)};
        add.set_coeff(std::move(coeff));
      }
      return add_expr;
    }

    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_add const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class n_ary_mul_add final : public add_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<scalar_expression>;
  using expr_holder_t = expression_holder<scalar_expression>;

  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  n_ary_mul_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_mul>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  // constant*expr + expr --> (constant+1)*expr
  auto dispatch([[maybe_unused]] scalar const &rhs) {
    // const auto &hash_lhs{lhs.hash_value()};
    const auto pos{lhs.hash_map().find(m_rhs)};
    if (pos != lhs.hash_map().end() && lhs.hash_map().size() == 1) {
      auto expr{make_expression<scalar_mul>(lhs)};
      auto &mul{expr.template get<scalar_mul>()};
      mul.set_coeff(
          make_expression<scalar_constant>(get_coefficient(lhs, 1) + 1));
      return expr;
    }
    return get_default();
  }

  /// expr + expr --> 2*expr
  auto dispatch(scalar_mul const &rhs) {
    const auto &hash_rhs{rhs.hash_value()};
    const auto &hash_lhs{lhs.hash_value()};
    if (hash_rhs == hash_lhs) {
      const auto fac_lhs{get_coefficient(lhs, 1)};
      const auto fac_rhs{get_coefficient(rhs, 1)};
      auto expr{make_expression<scalar_mul>(lhs)};
      auto &mul{expr.template get<scalar_mul>()};
      mul.set_coeff(make_expression<scalar_constant>(fac_lhs + fac_rhs));
      return expr;
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_mul const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class symbol_add final : public add_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<scalar_expression>;
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  symbol_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_type operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_type operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  /// x+x --> 2*x
  inline expr_type dispatch(scalar const &rhs) {
    if (lhs == rhs) {
      auto mul{make_expression<scalar_mul>()};
      mul.template get<scalar_mul>().set_coeff(
          make_expression<scalar_constant>(2));
      mul.template get<scalar_mul>().push_back(m_rhs);
      return mul;
    }
    return get_default();
  }

  inline expr_type dispatch(scalar_mul const &rhs) {
    // const auto &hash_rhs{rhs.hash_value()};
    const auto pos{rhs.hash_map().find(m_lhs)};
    if (pos != rhs.hash_map().end() && rhs.hash_map().size() == 1) {
      auto expr{make_expression<scalar_mul>(rhs)};
      auto &mul{expr.template get<scalar_mul>()};
      mul.set_coeff(
          make_expression<scalar_constant>(get_coefficient(rhs, 1) + 1));
      return expr;
    }
    return get_default();
  }
  //  inline expr_type operator()(scalar_constant
  //  const&rhs) {
  //  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class add_negative final : public add_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<scalar_expression>;
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  add_negative(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_negative>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  // (-lhs) + (-rhs) --> -(lhs+rhs)
  inline expr_type dispatch(scalar_negative const &rhs) {
    return -(lhs.expr() + rhs.expr());
  }

  inline expr_type dispatch([[maybe_unused]] scalar_add const &rhs) {
    auto add_expr{make_expression<scalar_add>(rhs)};
    auto &add{add_expr.template get<scalar_add>()};
    scalar_number c_lhs;
    if (is_same<scalar_constant>(lhs.expr())) {
      c_lhs = lhs.expr().get<scalar_constant>().value();
    }
    if (is_same<scalar_one>(lhs.expr())) {
      c_lhs = 1;
    }
    if (c_lhs != 0) {
      const auto value{get_coefficient(rhs, 0) - c_lhs};
      add.coeff().free();
      if (value != 0) {
        auto coeff{make_expression<scalar_constant>(value)};
        add.set_coeff(std::move(coeff));
        return add_expr;
      }
      return add_expr;
    }
    add.push_back(m_lhs);
    return add_expr;
  }

  // -expr + c
  inline expr_type dispatch(scalar_constant const &rhs) {
    if (is_same<scalar_constant>(lhs.expr())) {
      return make_expression<scalar_constant>(
          -lhs.expr().get<scalar_constant>().value() + rhs.value());
    }
    if (is_same<scalar_one>(lhs.expr())) {
      return make_expression<scalar_constant>(-1 + rhs.value());
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_negative const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
struct add_base final : public scalar_visitor_return_expr_t {
  using expr_t = scalar_expression;
  using expr_holder_t = expression_holder<scalar_expression>;

  add_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

private:
  expr_holder_t dispatch(scalar_constant const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    constant_add<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                           std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t dispatch(scalar_one const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    add_scalar_one<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                             std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t dispatch(scalar_add const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    n_ary_add<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                        std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t dispatch(scalar_mul const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    n_ary_mul_add<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                            std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t dispatch(scalar const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    symbol_add<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                         std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t dispatch(scalar_negative const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    add_negative<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                           std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t dispatch(scalar_zero const &) {
    return std::forward<ExprRHS>(m_rhs);
  }

  template <typename Type> expr_holder_t dispatch(Type const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    add_default<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                          std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};
} // namespace simplifier

} // namespace numsim::cas
#endif // SCALAR_SIMPLIFIER_ADD_H
