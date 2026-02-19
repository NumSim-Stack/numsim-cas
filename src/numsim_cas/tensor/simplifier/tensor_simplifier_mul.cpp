#include <numsim_cas/core/operators.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor/simplifier/tensor_simplifier_mul.h>
#include <numsim_cas/tensor/tensor_operators.h>

namespace numsim::cas {
namespace tensor_detail {
namespace simplifier {

tensor_pow_mul::tensor_pow_mul(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<tensor_pow>()} {}

tensor_pow_mul::expr_holder_t
tensor_pow_mul::dispatch([[maybe_unused]] tensor const &rhs) {
  if (lhs.expr_lhs().get().hash_value() == rhs.hash_value()) {
    const auto rhs_expr{lhs.expr_rhs() + get_scalar_one()};
    return make_expression<tensor_pow>(lhs.expr_lhs(), std::move(rhs_expr));
  }
  return get_default();
}

tensor_pow_mul::expr_holder_t
tensor_pow_mul::dispatch([[maybe_unused]] tensor_pow const &rhs) {
  if (lhs.hash_value() == rhs.hash_value()) {
    const auto rhs_expr{lhs.expr_rhs() + rhs.expr_rhs()};
    return make_expression<tensor_pow>(lhs.expr_lhs(), std::move(rhs_expr));
  }
  return get_default();
}

kronecker_delta_mul::kronecker_delta_mul(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<kronecker_delta>()} {}

// I_ij*expr_jkmnop.... --> expr_ikmnop....
template <typename Expr>
kronecker_delta_mul::expr_holder_t
kronecker_delta_mul::dispatch([[maybe_unused]] Expr const &rhs) {
  return std::move(m_rhs);
}

symbol_mul::symbol_mul(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<tensor>()} {}

/// X*X --> pow(X,2)
symbol_mul::expr_holder_t symbol_mul::dispatch(tensor const &rhs) {
  if (&lhs == &rhs) {
    return pow(std::move(m_lhs), 2);
  }
  return get_default();
}

/// X * pow(X,expr) --> pow(X,expr+1)
symbol_mul::expr_holder_t symbol_mul::dispatch(tensor_pow const &rhs) {
  if (lhs.hash_value() == rhs.expr_lhs().get().hash_value()) {
    return pow(m_lhs, rhs.expr_rhs() + get_scalar_one());
  }
  return get_default();
}

n_ary_mul::n_ary_mul(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<tensor_mul>()} {}

// check if last element == rhs; if not just pushback element
template <typename Expr>
n_ary_mul::expr_holder_t n_ary_mul::dispatch([[maybe_unused]] Expr const &rhs) {
  auto expr_mul{make_expression<tensor_mul>(lhs)};
  auto &mul{expr_mul.template get<tensor_mul>()};
  if (!lhs.data().empty() && lhs.data().back() == m_rhs) {
    auto last_element{lhs.data().back()};
    mul.data().erase(mul.data().end() - 1);
    return std::move(expr_mul) * (last_element * m_rhs);
  }
  mul.push_back(m_rhs);
  return expr_mul;
}

// merge to tensor_mul objects
n_ary_mul::expr_holder_t
n_ary_mul::dispatch([[maybe_unused]] tensor_mul const &rhs) {
  auto expr_mul{make_expression<tensor_mul>(lhs)};
  auto &mul{expr_mul.template get<tensor_mul>()};
  for (const auto &expr : rhs.data()) {
    mul.push_back(expr);
  }
  return expr_mul;
}

mul_base::mul_base(mul_base::expr_holder_t lhs, mul_base::expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

mul_base::expr_holder_t mul_base::dispatch(tensor const &) {
  auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
  symbol_mul visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

mul_base::expr_holder_t mul_base::dispatch(tensor_zero const &) {
  return m_lhs;
}

mul_base::expr_holder_t mul_base::dispatch(tensor_pow const &) {
  auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
  tensor_pow_mul visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

mul_base::expr_holder_t mul_base::dispatch(kronecker_delta const &) {
  auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
  kronecker_delta_mul visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

mul_base::expr_holder_t mul_base::dispatch(tensor_mul const &) {
  auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
  n_ary_mul visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

} // namespace simplifier
} // namespace tensor_detail
} // namespace numsim::cas
