#include <numsim_cas/core/operators.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/simplifier/scalar_simplifier_sub.h>

namespace numsim::cas {
namespace simplifier {

negative_sub::negative_sub(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<scalar_negative>()} {}

//-expr - (constant + x)
//-expr - constant - x
//-(expr + constant + x)
negative_sub::expr_holder_t
negative_sub::dispatch([[maybe_unused]] scalar_add const &rhs) {
  auto add_expr{make_expression<scalar_add>(rhs)};
  auto &add{add_expr.template get<scalar_add>()};
  auto coeff{base::m_lhs + add.coeff()};
  add.set_coeff(std::move(coeff));
  return make_expression<scalar_negative>(std::move(add_expr));
}

constant_sub::constant_sub(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<scalar_constant>()} {}

// lhs - rhs
constant_sub::expr_holder_t constant_sub::dispatch(scalar_constant const &rhs) {
  const auto value{lhs.value() - rhs.value()};
  return make_expression<scalar_constant>(value);
}

// constant_lhs - (constant + x)
// constant_lhs - constant - x
constant_sub::expr_holder_t
constant_sub::dispatch([[maybe_unused]] scalar_add const &rhs) {
  auto add_expr{make_expression<scalar_add>(rhs)};
  auto &add{add_expr.template get<scalar_add>()};
  auto coeff{base::m_lhs - add.coeff()};
  add.set_coeff(std::move(coeff));
  return add_expr;
}

constant_sub::expr_holder_t constant_sub::dispatch(scalar_one const &) {
  const auto value{lhs.value() - 1};
  return make_expression<scalar_constant>(value);
}

n_ary_sub::n_ary_sub(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<scalar_add>()} {}

n_ary_sub::expr_holder_t
n_ary_sub::dispatch([[maybe_unused]] scalar_constant const &rhs) {
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

n_ary_sub::expr_holder_t
n_ary_sub::dispatch([[maybe_unused]] scalar_one const &) {
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
n_ary_sub::expr_holder_t
n_ary_sub::dispatch([[maybe_unused]] scalar const &rhs) {
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
n_ary_sub::expr_holder_t n_ary_sub::dispatch(scalar_add const &rhs) {
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

n_ary_mul_sub::n_ary_mul_sub(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<scalar_mul>()} {}

// 2*x*y*z - x --> 2*x*y*z
// 2*x - x --> x
n_ary_mul_sub::expr_holder_t
n_ary_mul_sub::dispatch([[maybe_unused]] scalar const &rhs) {
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
n_ary_mul_sub::expr_holder_t n_ary_mul_sub::dispatch(scalar_mul const &rhs) {
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

symbol_sub::symbol_sub(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<scalar>()} {}

/// x-x --> 0
symbol_sub::expr_holder_t symbol_sub::dispatch(scalar const &rhs) {
  if (&lhs == &rhs) {
    return get_scalar_zero();
  }
  return get_default();
}

// x - 3*x --> -(2*x)
symbol_sub::expr_holder_t symbol_sub::dispatch(scalar_mul const &rhs) {
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

scalar_one_sub::scalar_one_sub(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<scalar_one>()} {}

scalar_one_sub::expr_holder_t
scalar_one_sub::dispatch(scalar_constant const &rhs) {
  return make_expression<scalar_constant>(1 - rhs.value());
}

sub_base::sub_base(expr_holder_t lhs, expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

sub_base::expr_holder_t sub_base::dispatch(scalar_constant const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  constant_sub visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

sub_base::expr_holder_t sub_base::dispatch(scalar_add const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  n_ary_sub visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

sub_base::expr_holder_t sub_base::dispatch(scalar_mul const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  n_ary_mul_sub visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

sub_base::expr_holder_t sub_base::dispatch(scalar const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  symbol_sub visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

sub_base::expr_holder_t sub_base::dispatch(scalar_one const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  scalar_one_sub visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

template <typename Type>
sub_base::expr_holder_t sub_base::dispatch([[maybe_unused]] Type const &rhs) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  sub_default<void> visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

// 0 - expr
sub_base::expr_holder_t sub_base::dispatch(scalar_zero const &) {
  return make_expression<scalar_negative>(std::move(m_rhs));
}

// - expr_lhs - expr_rhs --> -(expr_lhs+expr_rhs)
sub_base::expr_holder_t sub_base::dispatch(scalar_negative const &lhs) {
  return make_expression<scalar_negative>(lhs.expr() + std::move(m_rhs));
}

} // namespace simplifier
} // namespace numsim::cas
