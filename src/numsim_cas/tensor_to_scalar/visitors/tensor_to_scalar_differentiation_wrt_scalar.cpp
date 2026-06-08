#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_differentiation_wrt_scalar.h>

#include <numeric>
#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/diff.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/scalar/scalar_diff.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_diff.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>
#include <ranges>

namespace numsim::cas {

namespace {

// Wrap a plain scalar derivative as a t2s expression via the
// scalar_wrapper. Used by all the rules that compute a scalar-valued
// derivative.
inline expression_holder<tensor_to_scalar_expression>
wrap_scalar(expression_holder<scalar_expression> s) {
  return make_expression<tensor_to_scalar_scalar_wrapper>(std::move(s));
}

} // namespace

expression_holder<tensor_to_scalar_expression>
tensor_to_scalar_differentiation_wrt_scalar::apply(t2s_holder_t const &expr) {
  m_result = t2s_holder_t{};
  if (expr.is_valid()) {
    m_expr = expr;
    expr.get<tensor_to_scalar_visitable_t>().accept(*this);
  }
  if (!m_result.is_valid()) {
    // Pass-3 review: return the canonical tensor_to_scalar_zero
    // singleton (not a wrap_scalar(scalar_zero)) so that downstream
    // rules' `is_same<tensor_to_scalar_zero>` zero-detection works.
    // The wrapped form would silently bypass those checks and inflate
    // sums with redundant zero terms.
    return make_expression<tensor_to_scalar_zero>();
  }
  return m_result;
}

// Leaf constants: derivative is zero.
void tensor_to_scalar_differentiation_wrt_scalar::operator()(
    [[maybe_unused]] tensor_to_scalar_zero const &) {
  // m_result stays invalid -> apply returns scalar zero
}
void tensor_to_scalar_differentiation_wrt_scalar::operator()(
    [[maybe_unused]] tensor_to_scalar_one const &) {}

// Scalar wrapper: the inner scalar's derivative.
// d/ds(scalar_wrapper(s_inner)) = scalar_wrapper(diff(s_inner, s)).
// This is the bridge to the scalar-scalar diff machinery.
void tensor_to_scalar_differentiation_wrt_scalar::operator()(
    tensor_to_scalar_scalar_wrapper const &visitable) {
  auto ds_inner = diff(visitable.expr(), m_arg);
  if (ds_inner.is_valid() && !is_same<scalar_zero>(ds_inner)) {
    m_result = wrap_scalar(std::move(ds_inner));
  }
}

void tensor_to_scalar_differentiation_wrt_scalar::operator()(
    tensor_to_scalar_negative const &visitable) {
  auto df = diff(visitable.expr(), m_arg);
  if (df.is_valid() && !is_same<tensor_to_scalar_zero>(df)) {
    m_result = -df;
  }
}

void tensor_to_scalar_differentiation_wrt_scalar::operator()(
    tensor_to_scalar_add const &visitable) {
  t2s_holder_t sum;
  for (auto &child : visitable.symbol_map() | std::views::values) {
    auto d = diff(child, m_arg);
    if (d.is_valid() && !is_same<tensor_to_scalar_zero>(d)) {
      if (sum.is_valid()) {
        sum += d;
      } else {
        sum = std::move(d);
      }
    }
  }
  m_result = std::move(sum);
}

// Product rule: d(c * prod ai)/ds = c * sum_j (daj/ds * prod_{i!=j} ai)
void tensor_to_scalar_differentiation_wrt_scalar::operator()(
    tensor_to_scalar_mul const &visitable) {
  auto const &factors = visitable.symbol_map();

  t2s_holder_t sum;

  for (auto it_out = factors.begin(); it_out != factors.end(); ++it_out) {
    auto d_aj = diff(it_out->second, m_arg);
    if (!d_aj.is_valid() || is_same<tensor_to_scalar_zero>(d_aj)) {
      continue;
    }

    // Build prod_{i != j} ai
    t2s_holder_t rest;
    for (auto it_in = factors.begin(); it_in != factors.end(); ++it_in) {
      if (it_in == it_out)
        continue;
      if (rest.is_valid()) {
        rest = rest * it_in->second;
      } else {
        rest = it_in->second;
      }
    }

    t2s_holder_t term = std::move(d_aj);
    if (rest.is_valid()) {
      term = term * std::move(rest);
    }

    if (sum.is_valid()) {
      sum += term;
    } else {
      sum = std::move(term);
    }
  }

  // Apply outer coefficient if present.
  if (visitable.coeff().is_valid() && sum.is_valid()) {
    m_result = visitable.coeff() * std::move(sum);
  } else {
    m_result = std::move(sum);
  }
}

// pow(g, h): general case d(g^h) = g^(h-1) * (h*dg + dh*log(g)*g).
// When h is constant w.r.t. s (most common case), collapses to
//   d(g^h) = h * g^(h-1) * dg.
void tensor_to_scalar_differentiation_wrt_scalar::operator()(
    tensor_to_scalar_pow const &visitable) {
  auto const &g = visitable.expr_lhs();
  auto const &h = visitable.expr_rhs();

  auto dg = diff(g, m_arg);
  auto dh = diff(h, m_arg);

  const bool dg_is_zero = !dg.is_valid() || is_same<tensor_to_scalar_zero>(dg);
  const bool dh_is_zero = !dh.is_valid() || is_same<tensor_to_scalar_zero>(dh);
  if (dg_is_zero && dh_is_zero) {
    return;
  }

  auto one = wrap_scalar(get_scalar_one());

  if (dh_is_zero) {
    // Constant exponent: h * g^(h-1) * dg
    auto g_pow = pow(g, h - one);
    m_result = h * g_pow * std::move(dg);
  } else {
    // General: g^(h-1) * (h*dg + dh*log(g)*g)
    auto g_pow = pow(g, h - one);
    t2s_holder_t bracket;
    if (!dg_is_zero) {
      bracket = h * std::move(dg);
    }
    auto dh_term = std::move(dh) * log(g) * g;
    if (bracket.is_valid()) {
      bracket += dh_term;
    } else {
      bracket = std::move(dh_term);
    }
    m_result = std::move(g_pow) * std::move(bracket);
  }
}

// log(g): d(log(g))/ds = dg/ds / g
void tensor_to_scalar_differentiation_wrt_scalar::operator()(
    tensor_to_scalar_log const &visitable) {
  auto dg = diff(visitable.expr(), m_arg);
  if (!dg.is_valid() || is_same<tensor_to_scalar_zero>(dg)) {
    return;
  }
  auto inv_g = pow(visitable.expr(), -wrap_scalar(get_scalar_one()));
  m_result = std::move(dg) * inv_g;
}

// exp(g): d(exp(g))/ds = exp(g) * dg/ds
void tensor_to_scalar_differentiation_wrt_scalar::operator()(
    tensor_to_scalar_exp const &visitable) {
  auto dg = diff(visitable.expr(), m_arg);
  if (!dg.is_valid() || is_same<tensor_to_scalar_zero>(dg)) {
    return;
  }
  m_result = m_expr * std::move(dg);
}

// sqrt(g): d(sqrt(g))/ds = dg/ds / (2*sqrt(g))
void tensor_to_scalar_differentiation_wrt_scalar::operator()(
    tensor_to_scalar_sqrt const &visitable) {
  auto dg = diff(visitable.expr(), m_arg);
  if (!dg.is_valid() || is_same<tensor_to_scalar_zero>(dg)) {
    return;
  }
  auto two = wrap_scalar(make_expression<scalar_constant>(2));
  auto inv_2sqrt = pow(two * m_expr, -wrap_scalar(get_scalar_one()));
  m_result = std::move(dg) * inv_2sqrt;
}

// trace(A): d(trace(A))/ds = trace(dA/ds)
void tensor_to_scalar_differentiation_wrt_scalar::operator()(
    tensor_trace const &visitable) {
  auto dA = diff(visitable.expr(), m_arg);
  if (!dA.is_valid() || is_same<tensor_zero>(dA)) {
    return;
  }
  m_result = trace(std::move(dA));
}

// dot(A) = A:A. d/ds = 2 * (A : dA/ds).
void tensor_to_scalar_differentiation_wrt_scalar::operator()(
    tensor_dot const &visitable) {
  auto dA = diff(visitable.expr(), m_arg);
  if (!dA.is_valid() || is_same<tensor_zero>(dA)) {
    return;
  }
  auto rank = visitable.expr().get().rank();
  // 0-based iota: sequence::begin() exposes the 0-based internal
  // storage directly (the 1-based subtraction is only applied by
  // the initializer-list ctor, not the size_t one). Matches the
  // existing tensor-arg t2s sibling at
  // tensor_to_scalar_differentiation.cpp:217-218.
  sequence idx_a(rank), idx_b(rank);
  std::iota(idx_a.begin(), idx_a.end(), std::size_t{0});
  std::iota(idx_b.begin(), idx_b.end(), std::size_t{0});
  auto contraction = dot_product(visitable.expr(), std::move(idx_a),
                                 std::move(dA), std::move(idx_b));
  m_result =
      wrap_scalar(make_expression<scalar_constant>(2)) * std::move(contraction);
}

// norm(A) = sqrt(A:A). d/ds = (A : dA/ds) / norm(A).
void tensor_to_scalar_differentiation_wrt_scalar::operator()(
    tensor_norm const &visitable) {
  auto dA = diff(visitable.expr(), m_arg);
  if (!dA.is_valid() || is_same<tensor_zero>(dA)) {
    return;
  }
  auto rank = visitable.expr().get().rank();
  sequence idx_a(rank), idx_b(rank);
  std::iota(idx_a.begin(), idx_a.end(), std::size_t{0});
  std::iota(idx_b.begin(), idx_b.end(), std::size_t{0});
  auto contraction = dot_product(visitable.expr(), std::move(idx_a),
                                 std::move(dA), std::move(idx_b));
  auto inv_norm = pow(m_expr, -wrap_scalar(get_scalar_one()));
  m_result = std::move(contraction) * inv_norm;
}

// det(A): d/ds = det(A) * (inv(A^T) : dA/ds)  (Jacobi's formula).
void tensor_to_scalar_differentiation_wrt_scalar::operator()(
    tensor_det const &visitable) {
  auto dA = diff(visitable.expr(), m_arg);
  if (!dA.is_valid() || is_same<tensor_zero>(dA)) {
    return;
  }
  auto invAT = inv(trans(visitable.expr()));
  auto contracted =
      dot_product(invAT, sequence{1, 2}, std::move(dA), sequence{1, 2});
  m_result = m_expr * std::move(contracted);
}

// inner_product_to_scalar(A, idxA, B, idxB): product rule, t2s-valued.
// d/ds = inner(dA, idxA, B, idxB) + inner(A, idxA, dB, idxB)
void tensor_to_scalar_differentiation_wrt_scalar::operator()(
    tensor_inner_product_to_scalar const &visitable) {
  auto dA = diff(visitable.expr_lhs(), m_arg);
  auto dB = diff(visitable.expr_rhs(), m_arg);

  sequence const &seq_lhs = visitable.indices_lhs();
  sequence const &seq_rhs = visitable.indices_rhs();

  t2s_holder_t sum;
  if (dA.is_valid() && !is_same<tensor_zero>(dA)) {
    auto s_l = seq_lhs;
    auto s_r = seq_rhs;
    sum = dot_product(std::move(dA), std::move(s_l), visitable.expr_rhs(),
                      std::move(s_r));
  }
  if (dB.is_valid() && !is_same<tensor_zero>(dB)) {
    auto s_l = seq_lhs;
    auto s_r = seq_rhs;
    auto term = dot_product(visitable.expr_lhs(), std::move(s_l), std::move(dB),
                            std::move(s_r));
    if (sum.is_valid()) {
      sum += term;
    } else {
      sum = std::move(term);
    }
  }
  m_result = std::move(sum);
}

// if_then_else: piecewise. Parallel to the tensor-arg t2s visitor,
// throw not_implemented_error rather than approximate; the t2s
// condition / scalar-result wiring is the same wart as #241.
void tensor_to_scalar_differentiation_wrt_scalar::operator()(
    [[maybe_unused]] tensor_to_scalar_if_then_else const &) {
  throw not_implemented_error(
      "tensor_to_scalar_differentiation_wrt_scalar: "
      "tensor_to_scalar_if_then_else not yet implemented (#241)");
}

// Top-level CPO definition (declared in the header).
expression_holder<tensor_to_scalar_expression>
tag_invoke(detail::diff_fn, std::type_identity<tensor_to_scalar_expression>,
           std::type_identity<scalar_expression>,
           expression_holder<tensor_to_scalar_expression> const &expr,
           expression_holder<scalar_expression> const &arg) {
  tensor_to_scalar_differentiation_wrt_scalar d(arg);
  return d.apply(expr);
}

} // namespace numsim::cas
