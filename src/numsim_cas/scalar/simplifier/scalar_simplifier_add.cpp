#include <numsim_cas/core/operators.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/simplifier/scalar_simplifier_add.h>

namespace numsim::cas {
namespace simplifier {

// ------------------------------------------------------------
// constant_add
// ------------------------------------------------------------
constant_add::constant_add(expr_holder_t lhs_, expr_holder_t rhs_)
    : base(std::move(lhs_), std::move(rhs_)),
      lhs{base::m_lhs.template get<scalar_constant>()} {}

constant_add::expr_holder_t constant_add::dispatch(scalar_constant const &rhs) {
  const auto value{lhs.value() + rhs.value()};
  return make_expression<scalar_constant>(value);
}

constant_add::expr_holder_t
constant_add::dispatch([[maybe_unused]] scalar_add const &rhs) {
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

constant_add::expr_holder_t constant_add::dispatch(scalar_one const &) {
  const auto value{lhs.value() + 1};
  return make_expression<scalar_constant>(value);
}

constant_add::expr_holder_t constant_add::dispatch(scalar_negative const &rhs) {
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

// ------------------------------------------------------------
// add_scalar_one
// ------------------------------------------------------------
add_scalar_one::add_scalar_one(expr_holder_t lhs_, expr_holder_t rhs_)
    : base(std::move(lhs_), std::move(rhs_)),
      lhs{base::m_lhs.template get<scalar_one>()} {}

add_scalar_one::expr_holder_t
add_scalar_one::dispatch(scalar_constant const &rhs) {
  const auto value{1 + rhs.value()};
  if (value != 0)
    return make_expression<scalar_constant>(value);
  return get_scalar_zero();
}

add_scalar_one::expr_holder_t
add_scalar_one::dispatch([[maybe_unused]] scalar_add const &rhs) {
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

add_scalar_one::expr_holder_t add_scalar_one::dispatch(scalar_one const &) {
  const auto value{1 + 1};
  return make_expression<scalar_constant>(value);
}

add_scalar_one::expr_holder_t
add_scalar_one::dispatch(scalar_negative const &rhs) {
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

// ------------------------------------------------------------
// n_ary_add
// ------------------------------------------------------------
n_ary_add::n_ary_add(expr_holder_t lhs_, expr_holder_t rhs_)
    : base(std::move(lhs_), std::move(rhs_)),
      lhs{base::m_lhs.template get<scalar_add>()} {}

n_ary_add::expr_holder_t
n_ary_add::dispatch([[maybe_unused]] scalar_constant const &rhs) {
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

n_ary_add::expr_holder_t
n_ary_add::dispatch([[maybe_unused]] scalar_one const &) {
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

n_ary_add::expr_holder_t
n_ary_add::dispatch([[maybe_unused]] scalar const &rhs) {
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

n_ary_add::expr_holder_t n_ary_add::dispatch(scalar_add const &rhs) {
  auto expr{make_expression<scalar_add>()};
  auto &add{expr.template get<scalar_add>()};
  merge_add(lhs, rhs, add);
  return expr;
}

n_ary_add::expr_holder_t n_ary_add::dispatch(scalar_negative const &rhs) {
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

// ------------------------------------------------------------
// n_ary_mul_add
// ------------------------------------------------------------
n_ary_mul_add::n_ary_mul_add(expr_holder_t lhs_, expr_holder_t rhs_)
    : base(std::move(lhs_), std::move(rhs_)),
      lhs{base::m_lhs.template get<scalar_mul>()} {}

n_ary_mul_add::expr_holder_t
n_ary_mul_add::dispatch([[maybe_unused]] scalar const &rhs) {
  // constant*expr + expr --> (constant+1)*expr
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

n_ary_mul_add::expr_holder_t n_ary_mul_add::dispatch(scalar_mul const &rhs) {
  /// expr + expr --> 2*expr
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

// ------------------------------------------------------------
// symbol_add
// ------------------------------------------------------------
symbol_add::symbol_add(expr_holder_t lhs_, expr_holder_t rhs_)
    : base(std::move(lhs_), std::move(rhs_)),
      lhs{base::m_lhs.template get<scalar>()} {}

symbol_add::expr_holder_t symbol_add::dispatch(scalar const &rhs) {
  /// x+x --> 2*x
  if (lhs == rhs) {
    auto mul{make_expression<scalar_mul>()};
    mul.template get<scalar_mul>().set_coeff(
        make_expression<scalar_constant>(2));
    mul.template get<scalar_mul>().push_back(m_rhs);
    return mul;
  }
  return get_default();
}

symbol_add::expr_holder_t symbol_add::dispatch(scalar_mul const &rhs) {
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

// ------------------------------------------------------------
// add_negative
// ------------------------------------------------------------
add_negative::add_negative(expr_holder_t lhs_, expr_holder_t rhs_)
    : base(std::move(lhs_), std::move(rhs_)),
      lhs{base::m_lhs.template get<scalar_negative>()} {}

add_negative::expr_holder_t add_negative::dispatch(scalar_negative const &rhs) {
  // (-lhs) + (-rhs) --> -(lhs+rhs)
  return -(lhs.expr() + rhs.expr());
}

add_negative::expr_holder_t
add_negative::dispatch([[maybe_unused]] scalar_add const &rhs) {
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

add_negative::expr_holder_t add_negative::dispatch(scalar_constant const &rhs) {
  // -expr + c
  if (is_same<scalar_constant>(lhs.expr())) {
    return make_expression<scalar_constant>(
        -lhs.expr().get<scalar_constant>().value() + rhs.value());
  }
  if (is_same<scalar_one>(lhs.expr())) {
    return make_expression<scalar_constant>(-1 + rhs.value());
  }
  return get_default();
}

// ------------------------------------------------------------
// add_base
// ------------------------------------------------------------
add_base::add_base(expr_holder_t lhs, expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

add_base::expr_holder_t add_base::dispatch(scalar_constant const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  constant_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar_one const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  add_scalar_one visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar_add const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  n_ary_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar_mul const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  n_ary_mul_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  symbol_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar_negative const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  add_negative visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar_zero const &) {
  return std::move(m_rhs);
}

} // namespace simplifier
} // namespace numsim::cas
