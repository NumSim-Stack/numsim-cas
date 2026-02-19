#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_limit_visitor.h>

#include <numsim_cas/core/contains_expression.h>
#include <numsim_cas/scalar/visitors/scalar_limit_visitor.h>
#include <ranges>

namespace numsim::cas {

using dir = limit_result::direction;

// ─── Constructors ─────────────────────────────────────────────────

tensor_to_scalar_limit_visitor::tensor_to_scalar_limit_visitor(
    t2s_holder_t const &limit_var, limit_target target)
    : m_mode(dependency_mode::exact_match), m_limit_var_t2s(limit_var),
      m_target(target) {}

tensor_to_scalar_limit_visitor::tensor_to_scalar_limit_visitor(
    tensor_holder_t const &tensor_var, limit_target target)
    : m_mode(dependency_mode::tensor_dependency), m_tensor_var(tensor_var),
      m_target(target) {}

// ─── Apply ────────────────────────────────────────────────────────

limit_result
tensor_to_scalar_limit_visitor::apply(t2s_holder_t const &expr) {
  if (!expr.is_valid())
    return {dir::zero};

  // In exact match mode, check if this IS the limit variable
  if (m_mode == dependency_mode::exact_match && expr == m_limit_var_t2s) {
    using pt = limit_target::point;
    switch (m_target.target) {
    case pt::zero_plus:
      m_result = {dir::zero};
      break;
    case pt::zero_minus:
      m_result = {dir::zero};
      break;
    case pt::pos_infinity:
      m_result = {dir::pos_infinity, {growth_rate::type::polynomial, 1.0}};
      break;
    case pt::neg_infinity:
      m_result = {dir::neg_infinity, {growth_rate::type::polynomial, 1.0}};
      break;
    }
    return m_result;
  }

  expr.template get<tensor_to_scalar_visitable_t>().accept(*this);
  return m_result;
}

// ─── Dependency check ─────────────────────────────────────────────

bool tensor_to_scalar_limit_visitor::depends_on_limit_var(
    t2s_holder_t const &expr) const {
  if (m_mode == dependency_mode::exact_match) {
    // In exact mode: check if the expression IS or CONTAINS the limit var
    // Simple equality check for sub-expression
    if (expr.is_valid() && m_limit_var_t2s.is_valid() &&
        expr == m_limit_var_t2s)
      return true;
    return false;
  }
  // tensor_dependency mode: check if T2S expr depends on the tensor variable
  return depends_on_tensor(expr, m_tensor_var);
}

// ─── T2S functions ────────────────────────────────────────────────

void tensor_to_scalar_limit_visitor::operator()(
    [[maybe_unused]] tensor_trace const &v) {
  // trace depends on tensor child
  if (m_mode == dependency_mode::tensor_dependency) {
    if (contains_expression(v.expr(), m_tensor_var)) {
      // trace of tensor going to limit: unknown in general
      m_result = {dir::unknown};
      return;
    }
  }
  m_result = {dir::finite_positive};
}

void tensor_to_scalar_limit_visitor::operator()(
    [[maybe_unused]] tensor_dot const &v) {
  if (m_mode == dependency_mode::tensor_dependency) {
    if (contains_expression(v.expr(), m_tensor_var)) {
      m_result = {dir::unknown};
      return;
    }
  }
  m_result = {dir::finite_positive};
}

void tensor_to_scalar_limit_visitor::operator()(
    [[maybe_unused]] tensor_det const &v) {
  if (m_mode == dependency_mode::tensor_dependency) {
    if (contains_expression(v.expr(), m_tensor_var)) {
      // det depends on tensor: behavior depends on limit target
      // For generic analysis, return unknown
      m_result = {dir::unknown};
      return;
    }
  }
  m_result = {dir::finite_positive};
}

void tensor_to_scalar_limit_visitor::operator()(
    [[maybe_unused]] tensor_norm const &v) {
  if (m_mode == dependency_mode::tensor_dependency) {
    if (contains_expression(v.expr(), m_tensor_var)) {
      // norm(F) as F -> infinity => +infinity (polynomial)
      using pt = limit_target::point;
      if (m_target.target == pt::pos_infinity) {
        m_result = {dir::pos_infinity, {growth_rate::type::polynomial, 1.0}};
        return;
      }
      m_result = {dir::unknown};
      return;
    }
  }
  m_result = {dir::finite_positive};
}

void tensor_to_scalar_limit_visitor::operator()(
    tensor_inner_product_to_scalar const &v) {
  if (m_mode == dependency_mode::tensor_dependency) {
    bool lhs_dep = contains_expression(v.expr_lhs(), m_tensor_var);
    bool rhs_dep = contains_expression(v.expr_rhs(), m_tensor_var);
    if (lhs_dep || rhs_dep) {
      m_result = {dir::unknown};
      return;
    }
  }
  m_result = {dir::finite_positive};
}

// ─── Arithmetic ───────────────────────────────────────────────────

void tensor_to_scalar_limit_visitor::operator()(
    tensor_to_scalar_add const &v) {
  limit_result result{dir::zero};
  if (v.coeff().is_valid()) {
    result = apply(v.coeff());
  }
  for (auto const &child : v.hash_map() | std::views::values) {
    result = combine_add(result, apply(child));
  }
  m_result = result;
}

void tensor_to_scalar_limit_visitor::operator()(
    tensor_to_scalar_mul const &v) {
  limit_result result{dir::finite_positive};
  if (v.coeff().is_valid()) {
    result = apply(v.coeff());
  }
  for (auto const &child : v.hash_map() | std::views::values) {
    result = combine_mul(result, apply(child));
  }
  m_result = result;
}

void tensor_to_scalar_limit_visitor::operator()(
    tensor_to_scalar_negative const &v) {
  m_result = apply_neg(apply(v.expr()));
}

void tensor_to_scalar_limit_visitor::operator()(
    tensor_to_scalar_pow const &v) {
  m_result = apply_pow(apply(v.expr_lhs()), apply(v.expr_rhs()));
}

void tensor_to_scalar_limit_visitor::operator()(
    tensor_to_scalar_log const &v) {
  m_result = apply_log(apply(v.expr()));
}

// ─── Constants ────────────────────────────────────────────────────

void tensor_to_scalar_limit_visitor::operator()(
    [[maybe_unused]] tensor_to_scalar_zero const &) {
  m_result = {dir::zero};
}

void tensor_to_scalar_limit_visitor::operator()(
    [[maybe_unused]] tensor_to_scalar_one const &) {
  m_result = {dir::finite_positive};
}

void tensor_to_scalar_limit_visitor::operator()(
    tensor_to_scalar_scalar_wrapper const & /*v*/) {
  // Delegate to scalar limit visitor
  // Scalar expressions don't depend on tensor variables,
  // so they should evaluate to finite
  if (m_mode == dependency_mode::exact_match && m_limit_var_t2s.is_valid()) {
    // In exact match mode with a T2S limit var, scalar sub-expressions
    // don't contain the T2S limit var, so they're finite
    m_result = {dir::finite_positive};
  } else {
    // In tensor dependency mode, scalar expressions are independent of tensor
    m_result = {dir::finite_positive};
  }
}

} // namespace numsim::cas
