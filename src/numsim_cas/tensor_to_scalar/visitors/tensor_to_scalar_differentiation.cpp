#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_differentiation.h>

#include <numsim_cas/core/diff.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_diff.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>
#include <numeric>
#include <ranges>

namespace numsim::cas {

// Zero and One: derivative is zero (handled by apply returning zero)
void tensor_to_scalar_differentiation::operator()(
    [[maybe_unused]] tensor_to_scalar_zero const &visitable) {
  // m_result stays invalid -> apply returns zero
}

void tensor_to_scalar_differentiation::operator()(
    [[maybe_unused]] tensor_to_scalar_one const &visitable) {
  // constant -> zero
}

void tensor_to_scalar_differentiation::operator()(
    [[maybe_unused]] tensor_to_scalar_scalar_wrapper const &visitable) {
  // pure scalar, no tensor dependency -> zero
}

// Negation: d(-f)/dX = -df/dX
void tensor_to_scalar_differentiation::operator()(
    tensor_to_scalar_negative const &visitable) {
  auto df = diff(visitable.expr(), m_arg);
  if (!is_same<tensor_zero>(df)) {
    m_result = -df;
  }
}

// Addition: d(sum ai)/dX = sum d(ai)/dX
// coeff is constant offset -> derivative is zero
void tensor_to_scalar_differentiation::operator()(
    tensor_to_scalar_add const &visitable) {
  tensor_holder_t sum;
  for (auto &child : visitable.hash_map() | std::views::values) {
    auto d = diff(child, m_arg);
    if (!is_same<tensor_zero>(d)) {
      sum += d;
    }
  }
  m_result = std::move(sum);
}

// Multiplication: product rule
// d(c * prod ai)/dX = c * sum_j (d(aj)/dX * prod_{i!=j} ai)
void tensor_to_scalar_differentiation::operator()(
    tensor_to_scalar_mul const &visitable) {
  auto const &factors = visitable.hash_map();

  tensor_holder_t sum;

  for (auto it_out = factors.begin(); it_out != factors.end(); ++it_out) {
    auto d_aj = diff(it_out->second, m_arg);
    if (is_same<tensor_zero>(d_aj)) {
      continue;
    }

    // d_aj is a tensor. Multiply by all other t2s factors.
    // t2s * tensor -> tensor (via tensor_to_scalar_with_tensor_mul)
    tensor_holder_t term = std::move(d_aj);
    for (auto it_in = factors.begin(); it_in != factors.end(); ++it_in) {
      if (it_in == it_out) continue;
      // it_in->second is a t2s expression, term is tensor
      // Create f * A node directly
      term = make_expression<tensor_to_scalar_with_tensor_mul>(
          std::move(term), it_in->second);
    }

    if (term.is_valid()) {
      sum += term;
    }
  }

  // Apply coefficient (constant t2s factor)
  if (visitable.coeff().is_valid() && sum.is_valid()) {
    m_result = make_expression<tensor_to_scalar_with_tensor_mul>(
        std::move(sum), visitable.coeff());
  } else {
    m_result = std::move(sum);
  }
}

// Power: d(g^h)/dX
// If h is constant w.r.t. X: d(g^h)/dX = h * g^{h-1} * dg/dX
// General: d(g^h)/dX = g^{h-1} * (h * dg/dX + dh/dX * log(g) * g)
void tensor_to_scalar_differentiation::operator()(
    tensor_to_scalar_pow const &visitable) {
  auto const &g = visitable.expr_lhs();
  auto const &h = visitable.expr_rhs();

  auto one = make_expression<tensor_to_scalar_one>();

  auto dg = diff(g, m_arg);
  auto dh = diff(h, m_arg);

  bool dh_is_zero = is_same<tensor_zero>(dh);
  bool dg_is_zero = is_same<tensor_zero>(dg);

  if (dg_is_zero && dh_is_zero) {
    return; // zero
  }

  if (dh_is_zero) {
    // h is constant: d(g^h)/dX = h * g^{h-1} * dg/dX
    auto g_pow = pow(g, h - one);
    // h * g^{h-1} is t2s, dg is tensor -> need t2s * tensor
    m_result = make_expression<tensor_to_scalar_with_tensor_mul>(
        dg, h * g_pow);
  } else {
    // General case: g^{h-1} * (h * dg + dh * log(g) * g)
    auto g_pow = pow(g, h - one);

    tensor_holder_t bracket;
    if (!dg_is_zero) {
      // h * dg (t2s * tensor)
      bracket = make_expression<tensor_to_scalar_with_tensor_mul>(dg, h);
    }
    // dh * log(g) * g (tensor * t2s * t2s)
    auto log_g_times_g = log(g) * g;
    auto dh_term =
        make_expression<tensor_to_scalar_with_tensor_mul>(dh, log_g_times_g);
    if (bracket.is_valid()) {
      bracket += dh_term;
    } else {
      bracket = std::move(dh_term);
    }

    // Multiply by g^{h-1}
    m_result = make_expression<tensor_to_scalar_with_tensor_mul>(
        std::move(bracket), g_pow);
  }
}

// Log: d(log(g))/dX = dg/dX / g = (1/g) * dg/dX
void tensor_to_scalar_differentiation::operator()(
    tensor_to_scalar_log const &visitable) {
  auto dg = diff(visitable.expr(), m_arg);
  if (is_same<tensor_zero>(dg)) {
    return;
  }
  // (1/g) * dg -> t2s * tensor
  auto one_over_g =
      pow(visitable.expr(), -get_scalar_one());
  m_result = make_expression<tensor_to_scalar_with_tensor_mul>(
      std::move(dg), one_over_g);
}

// Trace: d(tr(A))/dX = I : dA/dX = inner(I, {1,2}, dA, {1,2})
void tensor_to_scalar_differentiation::operator()(
    tensor_trace const &visitable) {
  auto dA = diff(visitable.expr(), m_arg);
  if (is_same<tensor_zero>(dA)) {
    return; // zero
  }
  // Special case: if dA is identity_tensor, I:I{4} = I
  if (is_same<identity_tensor>(dA)) {
    m_result = m_I;
    return;
  }
  m_result = inner_product(m_I, sequence{1, 2}, std::move(dA), sequence{1, 2});
}

// Dot: d(A:A)/dX = 2 * A : dA/dX = 2 * inner(A, idx, dA, idx)
void tensor_to_scalar_differentiation::operator()(
    tensor_dot const &visitable) {
  auto dA = diff(visitable.expr(), m_arg);
  if (is_same<tensor_zero>(dA)) {
    return;
  }

  // Special case: if dA is identity_tensor, inner(A, idx, I{4}, idx) = A
  if (is_same<identity_tensor>(dA)) {
    m_result = 2 * visitable.expr();
    return;
  }

  auto rank = visitable.expr().get().rank();
  sequence idx(rank);
  std::iota(idx.begin(), idx.end(), std::size_t{0});

  m_result = 2 * inner_product(visitable.expr(), idx, std::move(dA), idx);
}

// Norm: d(||A||)/dX = (A : dA/dX) / ||A||
void tensor_to_scalar_differentiation::operator()(
    tensor_norm const &visitable) {
  auto dA = diff(visitable.expr(), m_arg);
  if (is_same<tensor_zero>(dA)) {
    return;
  }

  auto rank = visitable.expr().get().rank();
  sequence idx(rank);
  std::iota(idx.begin(), idx.end(), std::size_t{0});

  auto contracted = inner_product(visitable.expr(), idx, std::move(dA), idx);
  // contracted is a tensor (rank = rank_arg), divide by norm (t2s)
  // tensor / t2s: use t2s * tensor with pow(norm, -1)
  auto inv_norm = pow(m_expr, -get_scalar_one());
  m_result = make_expression<tensor_to_scalar_with_tensor_mul>(
      std::move(contracted), inv_norm);
}

// Det: d(det(A))/dX = det(A) * inv(A^T) : dA/dX
void tensor_to_scalar_differentiation::operator()(
    tensor_det const &visitable) {
  auto dA = diff(visitable.expr(), m_arg);
  if (is_same<tensor_zero>(dA)) {
    return;
  }
  // det(A) * inner(inv(trans(A)), {1,2}, dA, {1,2})
  auto invAT = inv(trans(visitable.expr()));
  auto contracted =
      inner_product(invAT, sequence{1, 2}, std::move(dA), sequence{1, 2});
  // m_expr is the det(A) node (t2s), contracted is tensor
  m_result = make_expression<tensor_to_scalar_with_tensor_mul>(
      std::move(contracted), m_expr);
}

// Inner product to scalar: product rule
// d(inner(A, idx_a, B, idx_b))/dX
// = inner(dA, idx_a, B, idx_b) + inner(A, idx_a, dB, idx_b)
void tensor_to_scalar_differentiation::operator()(
    tensor_inner_product_to_scalar const &visitable) {
  auto dA = diff(visitable.expr_lhs(), m_arg);
  auto dB = diff(visitable.expr_rhs(), m_arg);

  // Stored indices are already 0-based; inner_product passes them through
  sequence seq_lhs(visitable.indices_lhs());
  sequence seq_rhs(visitable.indices_rhs());

  tensor_holder_t result;

  if (!is_same<tensor_zero>(dA)) {
    result = inner_product(std::move(dA), seq_lhs, visitable.expr_rhs(), seq_rhs);
  }
  if (!is_same<tensor_zero>(dB)) {
    auto term = inner_product(visitable.expr_lhs(), seq_lhs, std::move(dB), seq_rhs);
    if (result.is_valid()) {
      result += term;
    } else {
      result = std::move(term);
    }
  }

  m_result = std::move(result);
}

} // namespace numsim::cas
