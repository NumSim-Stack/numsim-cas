#include <numsim_cas/scalar/visitors/scalar_differentiation.h>

#include <numsim_cas/printer_base.h>
#include <numsim_cas/scalar/scalar_diff.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <ranges>

namespace numsim::cas {

void scalar_differentiation::operator()(
    scalar_named_expression const &visitable) {
  scalar_differentiation d(m_arg);
  expr_holder_t result{d.apply(visitable.expr())};
  if (result.is_valid()) {
    m_result = make_expression<scalar_named_expression>("d" + visitable.name(),
                                                        result);
  }
}

void scalar_differentiation::operator()(scalar_mul const &visitable) {
  // Initialize accumulators to identity elements so the first iteration's
  // `+=` / `*=` doesn't operate on an invalid holder. Previously these
  // were default-constructed (invalid) and relied on
  // expression_holder's operator+= safety net (which converts
  // `invalid += x` to `*this = x`). That safety net is real but it
  // means CALLERS who read `data()->...` directly on the accumulator
  // before the first iteration null-deref — surfaced as a segfault
  // in #305 when the positivity-propagation read() call landed in the
  // mul instrumentation. The is_valid() guard in read() is still
  // belt-and-suspenders, but eliminating the invalid intermediate
  // here is the principled fix.
  expr_holder_t expr_result = get_scalar_zero();
  for (auto &expr_out : visitable.symbol_map() | std::views::values) {
    expr_holder_t expr_result_in = get_scalar_one();
    for (auto &expr_in : visitable.symbol_map() | std::views::values) {
      if (expr_out == expr_in) {
        scalar_differentiation d(m_arg);
        expr_result_in *= d.apply(expr_in);
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
  // Identity-init: see scalar_mul comment above.
  expr_holder_t expr_result = get_scalar_zero();
  for (auto &child : visitable.symbol_map() | std::views::values) {
    scalar_differentiation d(m_arg);
    expr_result += d.apply(child);
  }
  m_result = std::move(expr_result);
}

void scalar_differentiation::operator()(scalar_negative const &visitable) {
  scalar_differentiation d(m_arg);
  // d.apply() is guaranteed valid (see apply() contract in header).
  // -scalar_zero short-circuits to scalar_zero in tag_invoke(neg_fn),
  // so the assignment is correct unconditionally — no need to
  // special-case zero. Also preserves the "visitor sets m_result to
  // a valid expression" invariant called out in the apply contract.
  m_result = -d.apply(visitable.expr());
}

void scalar_differentiation::operator()(scalar_pow const &visitable) {
  const auto &g{visitable.expr_lhs()};
  const auto &h{visitable.expr_rhs()};
  const auto &one{get_scalar_one()};

  auto dg{diff(g, m_arg)};
  auto dh{diff(h, m_arg)};

  bool dh_zero = !dh.is_valid() || is_same<scalar_zero>(dh);
  bool dg_zero = !dg.is_valid() || is_same<scalar_zero>(dg);

  if (dg_zero && dh_zero) {
    m_result = get_scalar_zero();
    return;
  }

  if (dh_zero) {
    // h is constant (w.r.t. m_arg)
    m_result = h * pow(g, h - one) * dg;
  } else if (dg_zero) {
    // g is constant (w.r.t. m_arg)
    m_result = pow(g, h) * dh * log(g);
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
  } else if (is_negative(visitable.expr()) ||
             is_nonpositive(visitable.expr())) {
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

// ─── max / min via if_then_else (#137 + #135) ──────────────────────
// d/dx max(a, b) = if_then_else(a > b, da/dx, db/dx)
// d/dx min(a, b) = if_then_else(a < b, da/dx, db/dx)
// Boundary a == b: sub-gradients agree almost everywhere, so picking
// either side is fine in practice. The constitutive-modelling use
// cases hit the boundary on a measure-zero set under time evolution.
void scalar_differentiation::operator()(scalar_max const &v) {
  scalar_differentiation inner_diff(m_arg);
  auto da = inner_diff.apply(v.expr_lhs());
  auto db = inner_diff.apply(v.expr_rhs());
  m_result = if_then_else(gt(v.expr_lhs(), v.expr_rhs()), std::move(da),
                          std::move(db));
}
void scalar_differentiation::operator()(scalar_min const &v) {
  scalar_differentiation inner_diff(m_arg);
  auto da = inner_diff.apply(v.expr_lhs());
  auto db = inner_diff.apply(v.expr_rhs());
  m_result = if_then_else(lt(v.expr_lhs(), v.expr_rhs()), std::move(da),
                          std::move(db));
}

// ─── if_then_else (#135) ───────────────────────────────────────────
// Assumes the condition does not depend on x; strictly there are
// Dirac contributions at the boundary that this rule ignores, but
// they vanish in practice for yield-function / contact-gap models.
//
// Asymmetry vs. scalar_evaluator: the evaluator is LAZY — it applies
// only the selected branch (load-bearing for damage models with
// undefined-on-the-other-arm expressions like log of a nonpositive
// argument). The diff visitor is EAGER because it's symbolic, not
// numeric: we don't know which arm a future evaluation will select,
// so we must build symbolic derivatives of both. Don't try to
// "fix" this by short-circuiting on a known cond — the diff rule
// must be valid for every cond value, not just the one being seen
// at this construction site.
void scalar_differentiation::operator()(scalar_if_then_else const &v) {
  scalar_differentiation inner_diff(m_arg);
  auto dt = inner_diff.apply(v.expr_then());
  auto de = inner_diff.apply(v.expr_else());
  m_result = if_then_else(v.expr_cond(), std::move(dt), std::move(de));
}

} // namespace numsim::cas
