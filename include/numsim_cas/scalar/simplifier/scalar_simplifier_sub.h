#ifndef SCALAR_SIMPLIFIER_SUB_H
#define SCALAR_SIMPLIFIER_SUB_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_globals.h>

namespace numsim::cas {
namespace simplifier {
template <typename ExprLHS, typename ExprRHS>
class sub_default : public scalar_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<scalar_expression>;

  sub_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  // rhs is negative
  auto get_default() {
    // std::cout<<m_lhs<<" "<<m_rhs<<std::endl;
    // std::cout<<"lhs id "<<m_lhs.get().id()<<" "<<"lhs id
    // "<<m_rhs.get().id()<<std::endl; std::cout<<m_lhs.get().hash_value()<<"
    // "<<m_rhs.get().hash_value()<<std::endl;

    if (m_lhs.get().hash_value() == m_rhs.get().hash_value()) {
      return get_scalar_zero();
    }

    const auto lhs_constant{is_same<scalar_constant>(m_lhs)};
    const auto rhs_constant{is_same<scalar_constant>(m_rhs)};
    auto add_new{make_expression<scalar_add>()};
    auto &add{add_new.template get<scalar_add>()};
    if (lhs_constant) {
      add.set_coeff(m_lhs);
    } else {
      add_new.template get<scalar_add>().push_back(m_lhs);
    }

    if (rhs_constant) {
      add.set_coeff(make_expression<scalar_constant>(
          -m_rhs.template get<scalar_constant>().value()));
    } else {
      add_new.template get<scalar_add>().push_back(-m_rhs);
    }
    return add_new;
  }

  template <typename Expr> inline expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

  // expr - 0 --> expr
  inline expr_holder_t dispatch(scalar_zero const &) { return m_lhs; }

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
  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class negative_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::dispatch;
  using base::get_coefficient;

  negative_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_negative>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  //-expr - (constant + x)
  //-expr - constant - x
  //-(expr + constant + x)
  inline expr_holder_t dispatch([[maybe_unused]] scalar_add const &rhs) {
    std::cout << "negative_sub" << std::endl;
    std::cout << m_lhs << " " << m_rhs << std::endl;
    auto add_expr{make_expression<scalar_add>(rhs)};
    auto &add{add_expr.template get<scalar_add>()};
    auto coeff{base::m_lhs + add.coeff()};
    add.set_coeff(std::move(coeff));
    return make_expression<scalar_negative>(std::move(add_expr));
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_negative const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class constant_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::dispatch;
  using base::get_coefficient;

  constant_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_constant>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  // lhs - rhs
  inline expr_holder_t dispatch(scalar_constant const &rhs) {
    const auto value{lhs.value() - rhs.value()};
    return make_expression<scalar_constant>(value);
  }

  // constant_lhs - (constant + x)
  // constant_lhs - constant - x
  inline expr_holder_t dispatch([[maybe_unused]] scalar_add const &rhs) {
    auto add_expr{make_expression<scalar_add>(rhs)};
    auto &add{add_expr.template get<scalar_add>()};
    auto coeff{base::m_lhs - add.coeff()};
    add.set_coeff(std::move(coeff));
    return add_expr;
  }

  inline expr_holder_t dispatch(scalar_one const &) {
    const auto value{lhs.value() - 1};
    return make_expression<scalar_constant>(value);
  }

private:
  scalar_constant const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class n_ary_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::dispatch;
  using base::get_coefficient;

  n_ary_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_add>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  inline expr_holder_t dispatch([[maybe_unused]] scalar_constant const &rhs) {
    auto add_expr{make_expression<scalar_add>(lhs)};
    auto &add{add_expr.template get<scalar_add>()};
    auto coeff{lhs.coeff() - m_rhs};
    if (is_same<scalar_zero>(coeff)) {
      add.coeff().free();
      return add_expr;
    }
    if (coeff.template get<scalar_constant>().value() == 0) {
      add.coeff().free();
      return add_expr;
    }
    add.set_coeff(std::move(coeff));
    return add_expr;
  }

  inline expr_holder_t dispatch([[maybe_unused]] scalar_one const &) {
    auto add_expr{make_expression<scalar_add>(lhs)};
    auto &add{add_expr.template get<scalar_add>()};
    const auto value{get_coefficient(add, 0) - 1};
    add.coeff().free();
    if (value != 0) {
      auto coeff{make_expression<scalar_constant>(value)};
      add.set_coeff(std::move(coeff));
    }
    return add_expr;
  }

  // x+y+z - x --> y+z
  auto dispatch([[maybe_unused]] scalar const &rhs) {
    /// do a deep copy of data
    auto expr_add{make_expression<scalar_add>(lhs)};
    auto &add{expr_add.template get<scalar_add>()};
    /// check if sub_exp == expr_rhs for sub_exp \in expr_lhs
    auto pos{add.hash_map().find(m_rhs)};
    if (pos != add.hash_map().end()) {
      auto expr{pos->second - m_rhs};
      add.hash_map().erase(pos);
      add.push_back(expr);
      return expr_add;
    }
    /// no equal expr or sub_expr
    add.push_back(-m_rhs);
    return expr_add;
  }

  // merge two expression
  auto dispatch(scalar_add const &rhs) {
    auto expr{make_expression<scalar_add>()};
    auto &add{expr.template get<scalar_add>()};
    add.set_coeff(lhs.coeff() + rhs.coeff());
    std::set<expr_holder_t> used_expr;
    for (auto &child : lhs.hash_map() | std::views::values) {
      auto pos{rhs.hash_map().find(child)};
      if (pos != rhs.hash_map().end()) {
        used_expr.insert(pos->second);
        add.push_back(child + pos->second);
      } else {
        add.push_back(child);
      }
    }
    if (used_expr.size() != rhs.size()) {
      for (auto &child : rhs.hash_map() | std::views::values) {
        if (!used_expr.count(child)) {
          add.push_back(child);
        }
      }
    }
    return expr;
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_add const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class n_ary_mul_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  n_ary_mul_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_mul>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  // 2*x*y*z - x --> 2*x*y*z
  // 2*x - x --> x
  auto dispatch([[maybe_unused]] scalar const &rhs) {
    if (lhs.hash_map().size() == 1) {
      auto pos{lhs.hash_map().find(m_rhs)};
      if (lhs.hash_map().end() != pos) {
        const auto value{get_coefficient(lhs, 0) - 1};
        if (value == 0) {
          return get_scalar_zero();
        }

        if (value == 1) {
          return m_rhs;
        }

        return make_expression<scalar_constant>(value) * m_rhs;
      }
    }

    return get_default();
  }

  /// expr + expr --> 2*expr
  auto dispatch(scalar_mul const &rhs) {
    const auto &hash_rhs{rhs.hash_value()};
    const auto &hash_lhs{lhs.hash_value()};
    if (hash_rhs == hash_lhs) {
      const auto fac_lhs{get_coefficient(lhs, 1.0)};
      const auto fac_rhs{get_coefficient(rhs, 1.0)};
      auto expr{make_expression<scalar_mul>(lhs)};
      auto &mul{expr.template get<scalar_mul>()};
      mul.set_coeff(make_expression<scalar_constant>(fac_lhs + fac_rhs));
      return std::move(expr);
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_mul const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class symbol_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  symbol_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  /// x-x --> 0
  inline expr_holder_t dispatch(scalar const &rhs) {
    if (&lhs == &rhs) {
      return get_scalar_zero();
    }
    return get_default();
  }

  // x - 3*x --> -(2*x)
  inline expr_holder_t dispatch(scalar_mul const &rhs) {
    const auto &hash_rhs{rhs.hash_value()};
    const auto &hash_lhs{lhs.hash_value()};
    if (hash_rhs == hash_lhs) {
      auto expr{make_expression<scalar_mul>(rhs)};
      auto &mul{expr.template get<scalar_mul>()};
      const auto value{1.0 - get_coefficient(rhs, 1.0)};
      mul.set_coeff(make_expression<scalar_constant>(value.abs()));
      if (value < 0) {
        return make_expression<scalar_negative>(std::move(expr));
      } else {
        return expr;
      }
    }
    return get_default();
  }

  //  inline expr_holder_t operator()(scalar_constant
  //  const&rhs) {
  //  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class scalar_one_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using expr_holder_t = expression_holder<scalar_expression>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::dispatch;
  using base::get_coefficient;
  using base::get_default;

  scalar_one_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_one>()} {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  inline expr_holder_t dispatch(scalar_constant const &rhs) {
    return make_expression<scalar_constant>(1 - rhs.value());
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_one const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
struct sub_base final : public scalar_visitor_return_expr_t {
  using expr_holder_t = expression_holder<scalar_expression>;

  sub_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  inline expr_holder_t dispatch(scalar_constant const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    constant_sub<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                           std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  inline expr_holder_t dispatch(scalar_add const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    n_ary_sub<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                        std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  inline expr_holder_t dispatch(scalar_mul const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    n_ary_mul_sub<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                            std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  inline expr_holder_t dispatch(scalar const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    symbol_sub<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                         std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  inline expr_holder_t dispatch(scalar_one const &) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    scalar_one_sub<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                             std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  template <typename Type>
  inline expr_holder_t dispatch([[maybe_unused]] Type const &rhs) {
    auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
    sub_default<ExprLHS, ExprRHS> visitor(std::forward<ExprLHS>(m_lhs),
                                          std::forward<ExprRHS>(m_rhs));
    return _rhs.accept(visitor);
  }

  // 0 - expr
  inline expr_holder_t dispatch(scalar_zero const &) {
    return make_expression<scalar_negative>(std::forward<ExprRHS>(m_rhs));
  }

  // - expr_lhs - expr_rhs --> -(expr_lhs+expr_rhs)
  inline expr_holder_t dispatch(scalar_negative const &lhs) {
    return make_expression<scalar_negative>(lhs.expr() +
                                            std::forward<ExprRHS>(m_rhs));
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

} // namespace simplifier
} // namespace numsim::cas

#endif // SCALAR_SIMPLIFIER_SUB_H
