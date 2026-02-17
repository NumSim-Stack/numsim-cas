#include <numsim_cas/core/operators.h>
#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_mul.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

namespace numsim::cas {
namespace tensor_to_scalar_detail {
namespace simplifier {

// wrapper_tensor_to_scalar_mul_mul::wrapper_tensor_to_scalar_mul_mul(expr_holder_t
// lhs, expr_holder_t rhs)
//     : base(std::move(lhs), std::move(rhs)),
//       lhs{base::m_lhs.template get<tensor_to_scalar_with_scalar_mul>()} {}

//        // tensor_to_scalar_with_scalar_mul * tensor_to_scalar_with_scalar_mul
//        -->
//        // tensor_to_scalar_with_scalar_mul
// [[nodiscard]] wrapper_tensor_to_scalar_mul_mul::expr_holder_t
// wrapper_tensor_to_scalar_mul_mul::dispatch(tensor_to_scalar_with_scalar_mul
// const &rhs) {
//   return make_expression<tensor_to_scalar_with_scalar_mul>(
//       lhs.expr_lhs() * rhs.expr_lhs(), lhs.expr_rhs() * rhs.expr_rhs());
// }

//        // tensor_to_scalar_with_scalar_mul * tensor_to_scalar -->
//        // tensor_to_scalar_with_scalar_mul
// template <typename Expr>
// [[nodiscard]] wrapper_tensor_to_scalar_mul_mul::expr_holder_t
// wrapper_tensor_to_scalar_mul_mul::dispatch(Expr const &) {
//   return make_expression<tensor_to_scalar_with_scalar_mul>(
//       lhs.expr_lhs(), m_rhs * lhs.expr_rhs());
// }

// --- constant_mul ---
using t2s_traits = domain_traits<tensor_to_scalar_expression>;

constant_mul::constant_mul(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs_val{t2s_traits::try_numeric(base::m_lhs)} {}

constant_mul::expr_holder_t
constant_mul::dispatch(tensor_to_scalar_scalar_wrapper const &) {
  auto rhs_val = t2s_traits::try_numeric(base::m_rhs);
  if (lhs_val && rhs_val) {
    return t2s_traits::make_constant(*lhs_val * *rhs_val);
  }
  return base::get_default();
}

constant_mul::expr_holder_t
constant_mul::dispatch(tensor_to_scalar_mul const &rhs) {
  if (lhs_val && *lhs_val == scalar_number{1}) {
    return std::move(m_rhs);
  }
  auto mul_expr{make_expression<tensor_to_scalar_mul>(rhs)};
  auto &mul{mul_expr.template get<tensor_to_scalar_mul>()};
  auto coeff{get_coefficient(mul, 1)};
  if (lhs_val) {
    coeff = coeff * *lhs_val;
  }
  mul.set_coeff(t2s_traits::make_constant(coeff));
  return mul_expr;
}

// --- mul_base helpers ---

// Try to fold a pow-with-numeric-base into an existing pow in the mul.
// Returns true if folded, false if caller should push_back normally.
static bool try_fold_numeric_pow(tensor_to_scalar_mul &mul,
                                 mul_base::expr_holder_t const &child) {
  if (!is_same<tensor_to_scalar_pow>(child))
    return false;
  auto const &pow_node = child.template get<tensor_to_scalar_pow>();
  auto child_base_val = t2s_traits::try_numeric(pow_node.expr_lhs());
  auto child_exp_val = t2s_traits::try_numeric(pow_node.expr_rhs());
  if (!child_base_val || !child_exp_val)
    return false;
  for (auto it = mul.hash_map().begin(); it != mul.hash_map().end(); ++it) {
    if (!is_same<tensor_to_scalar_pow>(it->second))
      continue;
    auto const &existing = it->second.template get<tensor_to_scalar_pow>();
    auto existing_base_val = t2s_traits::try_numeric(existing.expr_lhs());
    auto existing_exp_val = t2s_traits::try_numeric(existing.expr_rhs());
    if (existing_base_val && existing_exp_val &&
        *existing_exp_val == *child_exp_val) {
      auto combined_base =
          t2s_traits::make_constant(*existing_base_val * *child_base_val);
      auto combined = make_expression<tensor_to_scalar_pow>(
          std::move(combined_base), pow_node.expr_rhs());
      mul.hash_map().erase(it);
      mul.push_back(std::move(combined));
      return true;
    }
  }
  return false;
}

// Push child into mul, combining with existing entry if possible.
static void push_or_combine(tensor_to_scalar_mul &mul,
                             mul_base::expr_holder_t const &child) {
  if (try_fold_numeric_pow(mul, child))
    return;
  auto pos = mul.hash_map().find(child);
  if (pos != mul.hash_map().end()) {
    auto combined = pos->second * child;
    mul.hash_map().erase(pos);
    mul.push_back(std::move(combined));
    return;
  }
  mul.push_back(child);
}

// --- mul_base ---
mul_base::mul_base(expr_holder_t lhs, expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

[[nodiscard]] mul_base::expr_holder_t
mul_base::dispatch(tensor_to_scalar_mul const &lhs) {
  // mul * mul → merge (flatten)
  if (is_same<tensor_to_scalar_mul>(m_rhs)) {
    auto const &rhs_mul = m_rhs.template get<tensor_to_scalar_mul>();
    auto new_mul{make_expression<tensor_to_scalar_mul>(lhs)};
    auto &mul{new_mul.template get<tensor_to_scalar_mul>()};
    if (rhs_mul.coeff().is_valid()) {
      if (mul.coeff().is_valid()) {
        auto new_coeff = mul.coeff() * rhs_mul.coeff();
        mul.coeff().free();
        mul.set_coeff(std::move(new_coeff));
      } else {
        mul.set_coeff(rhs_mul.coeff());
      }
    }
    for (auto const &[k, v] : rhs_mul.hash_map()) {
      push_or_combine(mul, v);
    }
    return new_mul;
  }

  auto new_mul{make_expression<tensor_to_scalar_mul>(lhs)};
  auto &mul{new_mul.template get<tensor_to_scalar_mul>()};
  push_or_combine(mul, m_rhs);
  return new_mul;
}

// zero * expr --> zero
[[nodiscard]] mul_base::expr_holder_t
mul_base::dispatch(tensor_to_scalar_zero const &) {
  return m_lhs;
}

// one * expr --> expr
[[nodiscard]] mul_base::expr_holder_t
mul_base::dispatch(tensor_to_scalar_one const &) {
  return m_rhs;
}

// (-expr) * rhs --> -(expr * rhs)
[[nodiscard]] mul_base::expr_holder_t
mul_base::dispatch(tensor_to_scalar_negative const &lhs) {
  return -(lhs.expr() * m_rhs);
}

// scalar_wrapper * expr --> constant_mul visitor
[[nodiscard]] mul_base::expr_holder_t
mul_base::dispatch(tensor_to_scalar_scalar_wrapper const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  constant_mul visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas
