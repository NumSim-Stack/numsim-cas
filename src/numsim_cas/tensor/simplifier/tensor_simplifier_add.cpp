#include <numsim_cas/functions.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor/simplifier/tensor_simplifier_add.h>
#include <numsim_cas/tensor/tensor_operators.h>

namespace numsim::cas {
namespace simplifier {
namespace tensor_detail {

// ------------------------------------------------------------
// n_ary_add
// ------------------------------------------------------------
n_ary_add::n_ary_add(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs(m_lhs.template get<tensor_add>()) {}

template <typename Expr>
[[nodiscard]] auto
n_ary_add::dispatch([[maybe_unused]] Expr const &rhs) {
  auto expr_add{make_expression<tensor_add>(lhs)};
  auto &add{expr_add.template get<tensor_add>()};
  auto pos{lhs.hash_map().find(m_rhs)};
  if (pos != lhs.hash_map().end()) {
    add.hash_map().erase(m_rhs);
    add.push_back(pos->second + m_rhs);
    return expr_add;
  }
  add.push_back(m_rhs);
  return expr_add;
}

[[nodiscard]] auto n_ary_add::dispatch(tensor_add const &rhs) {
  auto expr{make_expression<tensor_add>(rhs.dim(), rhs.rank())};
  auto &add{expr.template get<tensor_add>()};
  merge_add(lhs, rhs, add);
  return expr;
}

[[nodiscard]] auto n_ary_add::dispatch(tensor_negative const &rhs) {
  const auto &expr_rhs{rhs.expr()};
  const auto pos{lhs.hash_map().find(expr_rhs)};
  if (pos != lhs.hash_map().end()) {
    auto expr{make_expression<tensor_add>(lhs)};
    auto &add{expr.template get<tensor_add>()};
    add.hash_map().erase(expr_rhs);
    return expr;
  }
  return get_default();
}

// ------------------------------------------------------------
// symbol_add
// ------------------------------------------------------------
symbol_add::symbol_add(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<tensor>()} {}

[[nodiscard]] symbol_add::expr_holder_t
symbol_add::dispatch(tensor const &rhs) {
  if (&lhs == &rhs) {
    return make_expression<tensor_scalar_mul>(
        make_expression<scalar_constant>(2), m_rhs);
  }
  return get_default();
}

// ------------------------------------------------------------
// tensor_scalar_mul_add
// ------------------------------------------------------------
tensor_scalar_mul_add::tensor_scalar_mul_add(expr_holder_t lhs,
                                             expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<tensor_scalar_mul>()} {}

template <typename Expr>
[[nodiscard]] tensor_scalar_mul_add::expr_holder_t
tensor_scalar_mul_add::dispatch(Expr const &rhs) {
  if (lhs.expr_rhs() == m_rhs) {
    return (lhs.expr_lhs() + 1) * m_rhs;
  }
  return get_default();
}

[[nodiscard]] tensor_scalar_mul_add::expr_holder_t
tensor_scalar_mul_add::dispatch(tensor_scalar_mul const &rhs) {
  if (lhs.expr_rhs().get().hash_value() == rhs.expr_rhs().get().hash_value()) {
    return (lhs.expr_lhs() + rhs.expr_lhs()) * rhs.expr_rhs();
  }
  return get_default();
}

// ------------------------------------------------------------
// add_negative
// ------------------------------------------------------------
add_negative::add_negative(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<tensor_negative>()} {}

[[nodiscard]] add_negative::expr_holder_t
add_negative::dispatch(tensor_negative const &rhs) {
  return -(lhs.expr() + rhs.expr());
}

// ------------------------------------------------------------
// add_base
// ------------------------------------------------------------
add_base::add_base(expr_holder_t lhs, expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

[[nodiscard]] add_base::expr_holder_t
add_base::dispatch(tensor_zero const &) {
  return std::move(m_rhs);
}

[[nodiscard]] add_base::expr_holder_t
add_base::dispatch(tensor_add const &) {
  auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
  n_ary_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

[[nodiscard]] add_base::expr_holder_t
add_base::dispatch(tensor_scalar_mul const &) {
  auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
  tensor_scalar_mul_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

[[nodiscard]] add_base::expr_holder_t
add_base::dispatch(tensor_negative const &) {
  auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
  add_negative visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

[[nodiscard]] add_base::expr_holder_t
add_base::dispatch(tensor const &) {
  auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
  symbol_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

} // namespace tensor_detail
} // namespace simplifier
} // namespace numsim::cas
