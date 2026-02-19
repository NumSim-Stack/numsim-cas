#include <numsim_cas/core/contains_expression.h>

#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_add.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_mul.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <ranges>

namespace numsim::cas {

// ─── Scalar containment visitor ───────────────────────────────────

namespace {

class scalar_contains_visitor final : public scalar_visitor_const_t {
public:
  using expr_holder_t = expression_holder<scalar_expression>;

  scalar_contains_visitor(expr_holder_t const &needle) : m_needle(needle) {}

  bool apply(expr_holder_t const &expr) {
    if (!expr.is_valid())
      return false;
    if (expr == m_needle)
      return true;
    m_found = false;
    expr.template get<scalar_visitable_t>().accept(*this);
    return m_found;
  }

  void operator()(scalar const &) override {}
  void operator()(scalar_zero const &) override {}
  void operator()(scalar_one const &) override {}
  void operator()(scalar_constant const &) override {}

  void operator()(scalar_add const &v) override {
    if (v.coeff().is_valid())
      check(v.coeff());
    for (auto const &child : v.hash_map() | std::views::values) {
      if (m_found)
        return;
      check(child);
    }
  }

  void operator()(scalar_mul const &v) override {
    if (v.coeff().is_valid())
      check(v.coeff());
    for (auto const &child : v.hash_map() | std::views::values) {
      if (m_found)
        return;
      check(child);
    }
  }

  void operator()(scalar_negative const &v) override { check(v.expr()); }
  void operator()(scalar_pow const &v) override {
    check(v.expr_lhs());
    if (!m_found)
      check(v.expr_rhs());
  }
  void operator()(scalar_rational const &v) override {
    check(v.expr_lhs());
    if (!m_found)
      check(v.expr_rhs());
  }
  void operator()(scalar_sin const &v) override { check(v.expr()); }
  void operator()(scalar_cos const &v) override { check(v.expr()); }
  void operator()(scalar_tan const &v) override { check(v.expr()); }
  void operator()(scalar_asin const &v) override { check(v.expr()); }
  void operator()(scalar_acos const &v) override { check(v.expr()); }
  void operator()(scalar_atan const &v) override { check(v.expr()); }
  void operator()(scalar_sqrt const &v) override { check(v.expr()); }
  void operator()(scalar_log const &v) override { check(v.expr()); }
  void operator()(scalar_exp const &v) override { check(v.expr()); }
  void operator()(scalar_sign const &v) override { check(v.expr()); }
  void operator()(scalar_abs const &v) override { check(v.expr()); }
  void operator()(scalar_function const &v) override { check(v.expr()); }

private:
  void check(expr_holder_t const &expr) {
    if (m_found)
      return;
    m_found = apply(expr);
  }

  expr_holder_t m_needle;
  bool m_found = false;
};

// ─── Tensor containment visitor ───────────────────────────────────

class tensor_contains_visitor final : public tensor_visitor_const_t {
public:
  using expr_holder_t = expression_holder<tensor_expression>;

  tensor_contains_visitor(expr_holder_t const &needle) : m_needle(needle) {}

  bool apply(expr_holder_t const &expr) {
    if (!expr.is_valid())
      return false;
    if (expr == m_needle)
      return true;
    m_found = false;
    expr.template get<tensor_visitable_t>().accept(*this);
    return m_found;
  }

  void operator()(tensor const &) override {}
  void operator()(tensor_zero const &) override {}
  void operator()(identity_tensor const &) override {}
  void operator()(kronecker_delta const &) override {}
  void operator()(tensor_projector const &) override {}

  void operator()(tensor_add const &v) override {
    if (v.coeff().is_valid())
      check(v.coeff());
    for (auto const &child : v.hash_map() | std::views::values) {
      if (m_found)
        return;
      check(child);
    }
  }

  void operator()(tensor_mul const &v) override {
    if (v.coeff().is_valid())
      check(v.coeff());
    for (auto const &child : v.data()) {
      if (m_found)
        return;
      check(child);
    }
  }

  void operator()(tensor_negative const &v) override { check(v.expr()); }
  void operator()(tensor_pow const &v) override { check(v.expr_lhs()); }
  void operator()(tensor_power_diff const &v) override { check(v.expr_lhs()); }
  void operator()(tensor_inv const &v) override { check(v.expr()); }

  void operator()(inner_product_wrapper const &v) override {
    check(v.expr_lhs());
    if (!m_found)
      check(v.expr_rhs());
  }
  void operator()(outer_product_wrapper const &v) override {
    check(v.expr_lhs());
    if (!m_found)
      check(v.expr_rhs());
  }
  void operator()(simple_outer_product const &v) override {
    for (auto const &child : v.data()) {
      if (m_found)
        return;
      check(child);
    }
  }
  void operator()(basis_change_imp const &v) override { check(v.expr()); }

  // tensor_scalar_mul: lhs is scalar, rhs is tensor
  void operator()(tensor_scalar_mul const &v) override { check(v.expr_rhs()); }

  // tensor_to_scalar_with_tensor_mul: lhs is tensor, rhs is T2S - only check
  // tensor child
  void operator()(tensor_to_scalar_with_tensor_mul const &v) override {
    check(v.expr_lhs());
  }

private:
  void check(expr_holder_t const &expr) {
    if (m_found)
      return;
    m_found = apply(expr);
  }

  expr_holder_t m_needle;
  bool m_found = false;
};

// ─── T2S depends_on_tensor visitor ────────────────────────────────

class t2s_depends_on_tensor_visitor final
    : public tensor_to_scalar_visitor_const_t {
public:
  using t2s_holder_t = expression_holder<tensor_to_scalar_expression>;
  using tensor_holder_t = expression_holder<tensor_expression>;

  t2s_depends_on_tensor_visitor(tensor_holder_t const &tensor_var)
      : m_tensor_var(tensor_var) {}

  bool apply(t2s_holder_t const &expr) {
    if (!expr.is_valid())
      return false;
    m_found = false;
    expr.template get<tensor_to_scalar_visitable_t>().accept(*this);
    return m_found;
  }

  // T2S operations with tensor children
  void operator()(tensor_trace const &v) override {
    check_tensor(v.expr());
  }
  void operator()(tensor_det const &v) override { check_tensor(v.expr()); }
  void operator()(tensor_norm const &v) override { check_tensor(v.expr()); }
  void operator()(tensor_dot const &v) override { check_tensor(v.expr()); }
  void operator()(tensor_inner_product_to_scalar const &v) override {
    check_tensor(v.expr_lhs());
    if (!m_found)
      check_tensor(v.expr_rhs());
  }

  // T2S arithmetic: recurse into T2S children
  void operator()(tensor_to_scalar_add const &v) override {
    if (v.coeff().is_valid())
      check_t2s(v.coeff());
    for (auto const &child : v.hash_map() | std::views::values) {
      if (m_found)
        return;
      check_t2s(child);
    }
  }
  void operator()(tensor_to_scalar_mul const &v) override {
    if (v.coeff().is_valid())
      check_t2s(v.coeff());
    for (auto const &child : v.hash_map() | std::views::values) {
      if (m_found)
        return;
      check_t2s(child);
    }
  }
  void operator()(tensor_to_scalar_negative const &v) override {
    check_t2s(v.expr());
  }
  void operator()(tensor_to_scalar_pow const &v) override {
    check_t2s(v.expr_lhs());
  }
  void operator()(tensor_to_scalar_log const &v) override {
    check_t2s(v.expr());
  }

  // Constants / scalar wrapper: no tensor dependency
  void operator()(tensor_to_scalar_zero const &) override {}
  void operator()(tensor_to_scalar_one const &) override {}
  void operator()(tensor_to_scalar_scalar_wrapper const &) override {}

private:
  void check_tensor(tensor_holder_t const &expr) {
    if (m_found)
      return;
    tensor_contains_visitor v(m_tensor_var);
    m_found = v.apply(expr);
  }

  void check_t2s(t2s_holder_t const &expr) {
    if (m_found)
      return;
    m_found = apply(expr);
  }

  tensor_holder_t m_tensor_var;
  bool m_found = false;
};

} // namespace

// ─── Public API ───────────────────────────────────────────────────

bool contains_expression(expression_holder<scalar_expression> const &haystack,
                         expression_holder<scalar_expression> const &needle) {
  scalar_contains_visitor v(needle);
  return v.apply(haystack);
}

bool contains_expression(expression_holder<tensor_expression> const &haystack,
                         expression_holder<tensor_expression> const &needle) {
  tensor_contains_visitor v(needle);
  return v.apply(haystack);
}

bool depends_on_tensor(
    expression_holder<tensor_to_scalar_expression> const &expr,
    expression_holder<tensor_expression> const &tensor_var) {
  t2s_depends_on_tensor_visitor v(tensor_var);
  return v.apply(expr);
}

} // namespace numsim::cas
