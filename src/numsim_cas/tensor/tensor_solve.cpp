#include <numsim_cas/tensor/tensor_solve.h>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/contains_expression.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_zero.h>
#include <numsim_cas/tensor/visitors/tensor_substitution.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_substitution.h>

namespace numsim::cas {

namespace {

using tensor_holder_t = expression_holder<tensor_expression>;
using scalar_holder_t = expression_holder<scalar_expression>;

// Check if D is identity_tensor (possibly scaled by a scalar or negated).
// Returns the scalar factor if so (1 for bare identity), nullopt otherwise.
std::optional<scalar_holder_t> try_identity_factor(tensor_holder_t const &D) {
  if (is_same<identity_tensor>(D)) {
    return get_scalar_one();
  }
  if (is_same<tensor_negative>(D)) {
    auto const &neg = D.get<tensor_negative>();
    auto inner = try_identity_factor(neg.expr());
    if (inner)
      return -(*inner);
    return std::nullopt;
  }
  if (is_same<tensor_scalar_mul>(D)) {
    auto const &sm = D.get<tensor_scalar_mul>();
    if (is_same<identity_tensor>(sm.expr_rhs())) {
      return sm.expr_lhs();
    }
  }
  return std::nullopt;
}

} // namespace

tensor_solver::tensor_solver(expr_holder_t const &expr, expr_holder_t const &x)
    : m_expr(expr), m_x(x) {}

std::vector<tensor_solver::expr_holder_t> tensor_solver::solve() const {
  // 1. Check if X appears in expr
  if (!contains_expression(m_expr, m_x))
    return {};

  // 2. Compute derivative (the "linear coefficient")
  //    D has rank = rank_expr + rank_X
  auto D = diff(m_expr, m_x);

  // 3. Check linearity: derivative must not depend on X
  if (contains_expression(D, m_x))
    return {}; // nonlinear

  // 4. Check D is not zero (degenerate)
  if (is_same<tensor_zero>(D))
    return {};

  // 5. Get constant term by substituting X = 0
  auto zero_tensor =
      make_expression<tensor_zero>(m_x.get().dim(), m_x.get().rank());
  auto b = substitute(m_expr, m_x, zero_tensor);

  // 6. Build solution
  auto neg_b = -b;

  // Fast path: D is (scalar * identity_tensor)
  // Then X = neg_b / scalar — no inv() needed
  if (auto factor = try_identity_factor(D)) {
    return {(get_scalar_one() / *factor) * neg_b};
  }

  // General case: D is not a scalar multiple of identity.
  // The symbolic inv(D) : b approach is not supported because tmech::inv
  // cannot handle rank-4 tensors without minor symmetry (e.g., the minor
  // identity delta_ik delta_jl). Return empty to signal unsolvable.
  return {};
}

std::vector<expression_holder<tensor_expression>>
solve(expression_holder<tensor_expression> const &expr,
      expression_holder<tensor_expression> const &x) {
  return tensor_solver(expr, x).solve();
}

} // namespace numsim::cas
