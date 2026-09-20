#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_limit_visitor.h>

#include <numsim_cas/core/contains_expression.h>
#include <numsim_cas/scalar/scalar_assume.h>
#include <numsim_cas/scalar/visitors/scalar_limit_visitor.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_domain_traits.h>
#include <ranges>

namespace numsim::cas {

using dir = limit_result::direction;

namespace {

// A value that does not depend on the limit variable: only its assumptions
// can give it a sign.
limit_result constant_sign(tensor_to_scalar_expression const &e) {
  if (e.assumptions().contains(positive{}))
    return {dir::finite_positive};
  if (e.assumptions().contains(negative{}))
    return {dir::finite_negative};
  return {dir::unknown};
}

// Integer value of a constant t2s expression, if it is one.
std::optional<std::int64_t>
int_constant(expression_holder<tensor_to_scalar_expression> const &e) {
  auto value = domain_traits<tensor_to_scalar_expression>::try_numeric(e);
  if (!value)
    return std::nullopt;
  if (auto const *i = std::get_if<std::int64_t>(&value->raw()))
    return *i;
  if (auto const *r = std::get_if<rational_t>(&value->raw())) {
    if (r->den == 1)
      return r->num;
  }
  if (auto const *d = std::get_if<double>(&value->raw())) {
    if (*d >= -9.2e18 && *d <= 9.2e18 &&
        *d == static_cast<double>(static_cast<std::int64_t>(*d)))
      return static_cast<std::int64_t>(*d);
  }
  return std::nullopt;
}

// Nonnegative by construction, so a limit of zero is approached from above.
bool is_structurally_nonnegative(
    expression_holder<tensor_to_scalar_expression> const &e) {
  if (is_same<tensor_to_scalar_exp>(e))
    return true;
  if (is_same<tensor_to_scalar_pow>(e)) {
    auto exponent = int_constant(e.get<tensor_to_scalar_pow>().expr_rhs());
    return exponent && *exponent % 2 == 0;
  }
  return false;
}

// A positive value, proven by an assumption or by a numeric constant. A
// wrapped scalar carries its facts on the scalar itself, not on the wrapper.
bool is_provably_positive(
    expression_holder<tensor_to_scalar_expression> const &e) {
  if (e.get().assumptions().contains(positive{}))
    return true;
  if (is_same<tensor_to_scalar_scalar_wrapper>(e) &&
      is_positive(e.get<tensor_to_scalar_scalar_wrapper>().expr()))
    return true;
  auto value = domain_traits<tensor_to_scalar_expression>::try_numeric(e);
  return value && numeric_less(scalar_number(std::int64_t{0}), *value);
}

} // namespace

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

limit_result tensor_to_scalar_limit_visitor::apply(t2s_holder_t const &expr) {
  if (!expr.is_valid())
    return {dir::zero};

  // In exact match mode, check if this IS the limit variable
  if (m_mode == dependency_mode::exact_match && expr == m_limit_var_t2s) {
    using pt = limit_target::point;
    switch (m_target.target) {
    case pt::zero_plus:
      [[fallthrough]];
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
  m_result = m_result.normalized();
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

bool tensor_to_scalar_limit_visitor::zero_from_above(
    t2s_holder_t const &expr) const {
  if (m_mode == dependency_mode::exact_match && expr == m_limit_var_t2s)
    return m_target.target == limit_target::point::zero_plus;
  // sqrt is nonnegative exactly where it is defined, so it reaches zero from
  // above only if its operand does
  if (is_same<tensor_to_scalar_sqrt>(expr))
    return zero_from_above(expr.get<tensor_to_scalar_sqrt>().expr());
  if (is_same<tensor_to_scalar_mul>(expr))
    return product_from_above(expr);
  return is_provably_positive(expr) || is_structurally_nonnegative(expr);
}

// A product stays positive near the limit when every factor does.
bool tensor_to_scalar_limit_visitor::product_from_above(
    t2s_holder_t const &expr) const {
  auto const &mul = expr.get<tensor_to_scalar_mul>();
  auto factor_stays_positive = [this](t2s_holder_t const &factor) {
    return is_provably_positive(factor) || zero_from_above(factor);
  };
  if (mul.coeff().is_valid() && !factor_stays_positive(mul.coeff()))
    return false;
  for (auto const &child : mul.symbol_map() | std::views::values) {
    if (!factor_stays_positive(child))
      return false;
  }
  return true;
}

// ─── T2S functions ────────────────────────────────────────────────

void tensor_to_scalar_limit_visitor::operator()(tensor_trace const &v) {
  // trace depends on tensor child
  if (m_mode == dependency_mode::tensor_dependency) {
    if (contains_expression(v.expr(), m_tensor_var)) {
      // trace of tensor going to limit: unknown in general
      m_result = {dir::unknown};
      return;
    }
  }
  m_result = constant_sign(v);
}

void tensor_to_scalar_limit_visitor::operator()(tensor_dot const &v) {
  if (m_mode == dependency_mode::tensor_dependency) {
    if (contains_expression(v.expr(), m_tensor_var)) {
      m_result = {dir::unknown};
      return;
    }
  }
  m_result = constant_sign(v);
}

void tensor_to_scalar_limit_visitor::operator()(tensor_det const &v) {
  if (m_mode == dependency_mode::tensor_dependency) {
    if (contains_expression(v.expr(), m_tensor_var)) {
      // det depends on tensor: behavior depends on limit target
      // For generic analysis, return unknown
      m_result = {dir::unknown};
      return;
    }
  }
  m_result = constant_sign(v);
}

void tensor_to_scalar_limit_visitor::operator()(tensor_norm const &v) {
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
  m_result = constant_sign(v);
}

void tensor_to_scalar_limit_visitor::operator()(
    tensor_to_scalar_eigenvalue const &v) {
  // An eigenvalue's sign and magnitude aren't recoverable from the AST
  // generically (unlike norm/det), so any tensor dependency is unknown.
  if (m_mode == dependency_mode::tensor_dependency &&
      contains_expression(v.expr(), m_tensor_var)) {
    m_result = {dir::unknown};
    return;
  }
  m_result = constant_sign(v);
}

void tensor_to_scalar_limit_visitor::operator()(
    tensor_to_scalar_divided_difference const &v) {
  // A divided difference of eigenvalues — like an eigenvalue, not
  // recoverable from the AST; any tensor dependency is unknown.
  if (m_mode == dependency_mode::tensor_dependency &&
      contains_expression(v.expr(), m_tensor_var)) {
    m_result = {dir::unknown};
    return;
  }
  m_result = constant_sign(v);
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
  m_result = constant_sign(v);
}

// ─── Arithmetic ───────────────────────────────────────────────────

void tensor_to_scalar_limit_visitor::operator()(tensor_to_scalar_add const &v) {
  limit_result result{dir::zero};
  if (v.coeff().is_valid()) {
    result = apply(v.coeff());
  }
  for (auto const &child : v.symbol_map() | std::views::values) {
    result = combine_add(result, apply(child));
  }
  m_result = result;
}

void tensor_to_scalar_limit_visitor::operator()(tensor_to_scalar_mul const &v) {
  limit_result result{dir::finite_positive};
  if (v.coeff().is_valid()) {
    result = apply(v.coeff());
  }
  for (auto const &child : v.symbol_map() | std::views::values) {
    result = combine_mul(result, apply(child));
  }
  m_result = result;
}

void tensor_to_scalar_limit_visitor::operator()(
    tensor_to_scalar_negative const &v) {
  m_result = apply_neg(apply(v.expr()));
}

void tensor_to_scalar_limit_visitor::operator()(tensor_to_scalar_pow const &v) {
  // An even integer exponent makes the power nonnegative from either side.
  auto exponent = int_constant(v.expr_rhs());
  const bool base_from_above = zero_from_above(v.expr_lhs());
  const bool from_above = base_from_above || (exponent && *exponent % 2 == 0);
  auto base = apply(v.expr_lhs());
  m_result = apply_pow(base, apply(v.expr_rhs()), from_above);
  // a non-integer power of a base that may reach zero from below is NaN
  if (base.dir == dir::zero && !base_from_above && !exponent)
    m_result = {dir::unknown};
}

void tensor_to_scalar_limit_visitor::operator()(tensor_to_scalar_log const &v) {
  m_result = apply_log(apply(v.expr()), zero_from_above(v.expr()));
}

void tensor_to_scalar_limit_visitor::operator()(tensor_to_scalar_exp const &v) {
  m_result = apply_exp(apply(v.expr()));
}

void tensor_to_scalar_limit_visitor::operator()(
    tensor_to_scalar_sqrt const &v) {
  m_result = apply_sqrt(apply(v.expr()), zero_from_above(v.expr()));
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
    tensor_to_scalar_scalar_wrapper const &v) {
  // A scalar sub-expression is constant with respect to the limit variable.
  auto const &e = v.expr();
  if (is_same<scalar_zero>(e))
    m_result = {dir::zero};
  else if (is_positive(e))
    m_result = {dir::finite_positive};
  else if (is_negative(e))
    m_result = {dir::finite_negative};
  else
    m_result = {dir::unknown};
}

// if_then_else (#135 / #210): limit depends on the condition's eventual
// behaviour near the limit target — out of scope for the current limit
// machinery. Report unknown, matching the scalar variant's behavior.
void tensor_to_scalar_limit_visitor::operator()(
    [[maybe_unused]] tensor_to_scalar_if_then_else const &) {
  m_result = {dir::unknown};
}

} // namespace numsim::cas
