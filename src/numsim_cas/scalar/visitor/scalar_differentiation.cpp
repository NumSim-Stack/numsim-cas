#include <numsim_cas/scalar/visitors/scalar_differentiation.h>

#include <numsim_cas/printer_base.h>
#include <numsim_cas/scalar/scalar_diff.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <ranges>

namespace numsim::cas {

void scalar_differentiation::operator()(
    scalar_named_expression const &visitable) {
  scalar_differentiation diff(m_arg);
  expr_holder_t result{diff.apply(visitable.expr())};
  if (result.is_valid()) {
    m_result = make_expression<scalar_named_expression>(
        "d" + visitable.name(), result);
  }
}

void scalar_differentiation::operator()(scalar_mul const &visitable) {
  expr_holder_t expr_result;
  for (auto &expr_out : visitable.hash_map() | std::views::values) {
    expr_holder_t expr_result_in;
    for (auto &expr_in : visitable.hash_map() | std::views::values) {
      if (expr_out == expr_in) {
        scalar_differentiation diff(m_arg);
        expr_result_in *= diff.apply(expr_in);
      } else {
        expr_result_in *= expr_in;
      }
    }
    expr_result += expr_result_in;
  }

  // NOTE: This assumes coeff() is constant w.r.t. m_arg
  if (visitable.coeff().is_valid()) {
    m_result = std::move(expr_result) * visitable.coeff();
  } else {
    m_result = std::move(expr_result);
  }
}

void scalar_differentiation::operator()(scalar_add const &visitable) {
  expr_holder_t expr_result;
  for (auto &child : visitable.hash_map() | std::views::values) {
    scalar_differentiation diff(m_arg);
    expr_result += diff.apply(child);
  }
  m_result = std::move(expr_result);
}

void scalar_differentiation::operator()(scalar_negative const &visitable) {
  scalar_differentiation diff(m_arg);
  auto diff_expr{diff.apply(visitable.expr())};
  if (diff_expr.is_valid() || !is_same<scalar_zero>(diff_expr)) {
    m_result = -diff_expr;
  }
}

void scalar_differentiation::operator()(scalar_pow const &visitable) {
  const auto &g{visitable.expr_lhs()};
  const auto &h{visitable.expr_rhs()};
  const auto &one{get_scalar_one()};

  auto dg{diff(g, m_arg)};
  auto dh{diff(h, m_arg)};

  if (is_same<scalar_zero>(dh)) {
    // h is constant (w.r.t. m_arg)
    m_result = h * pow(g, h - one) * dg;
  } else {
    // general case
    m_result = pow(g, h - one) * (h * dg + dh * log(g) * g);
  }
}

void scalar_differentiation::operator()(scalar_tan const &visitable) {
  const auto &one{get_scalar_one()};
  m_result = pow(one / cos(visitable.expr()), 2);
  apply_inner_unary(visitable);
}

void scalar_differentiation::operator()(scalar_sin const &visitable) {
  m_result = cos(visitable.expr());
  apply_inner_unary(visitable);
}

void scalar_differentiation::operator()(scalar_cos const &visitable) {
  m_result = -sin(visitable.expr());
  apply_inner_unary(visitable);
}

void scalar_differentiation::operator()(scalar_atan const &visitable) {
  auto &one{get_scalar_one()};
  m_result = (one / (one + pow(visitable.expr(), 2)));
  apply_inner_unary(visitable);
}

void scalar_differentiation::operator()(scalar_asin const &visitable) {
  auto &one{get_scalar_one()};
  m_result = (one / (sqrt(one - pow(visitable.expr(), 2))));
  apply_inner_unary(visitable);
}

void scalar_differentiation::operator()(scalar_acos const &visitable) {
  auto &one{get_scalar_one()};
  m_result = -(one / (sqrt(one - pow(visitable.expr(), 2))));
  apply_inner_unary(visitable);
}

void scalar_differentiation::operator()(scalar_sqrt const &visitable) {
  auto &one{get_scalar_one()};
  m_result = one / (2 * m_expr);
  apply_inner_unary(visitable);
}

void scalar_differentiation::operator()(scalar_exp const &visitable) {
  m_result = m_expr;
  apply_inner_unary(visitable);
}

void scalar_differentiation::operator()(scalar_abs const &visitable) {
  if (is_positive(visitable.expr()) || is_nonnegative(visitable.expr())) {
    // |u| = u when u >= 0, so d|u|/dx = u'
    m_result = get_scalar_one();
  } else if (is_negative(visitable.expr()) || is_nonpositive(visitable.expr())) {
    // |u| = -u when u <= 0, so d|u|/dx = -u'
    m_result = -get_scalar_one();
  } else {
    // General case: d|u|/dx = (u/|u|) * u'
    m_result = visitable.expr() / m_expr;
  }
  apply_inner_unary(visitable);
}

void scalar_differentiation::operator()(scalar_log const &visitable) {
  auto &one{get_scalar_one()};
  m_result = one / visitable.expr();
  apply_inner_unary(visitable);
}

} // namespace numsim::cas
