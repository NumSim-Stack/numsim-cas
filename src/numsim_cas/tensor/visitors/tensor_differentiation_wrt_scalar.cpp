#include <numsim_cas/tensor/visitors/tensor_differentiation_wrt_scalar.h>

#include <numeric>
#include <numsim_cas/core/diff.h>
#include <utility>
#include <numsim_cas/scalar/scalar_functions.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_diff.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

namespace numsim::cas {

// tensor_pow(A, n) where A is a tensor (assumed rank-2 matrix-product
// semantics; the tensor_pow node expands an integer exponent into
// repeated matrix multiplication). Scalar-arg derivative is the
// standard product rule:
//
//   d/ds(A^n) = sum_{r=0}^{n-1} A^r * dA/ds * A^{n-1-r}
//
// where * is the rank-2 matrix product (inner_product over the middle
// index). Result rank == A's rank, no rank expansion.
void tensor_differentiation_wrt_scalar::operator()(
    tensor_pow const &visitable) {
  auto const &A = visitable.expr_lhs();
  auto const &n_expr = visitable.expr_rhs();

  auto dA = diff(A, m_arg);
  // Pass-5 review: !is_valid() is dead since pass-3's t2s apply() fix
  // (and the tensor visitor's pre-existing apply() behaviour) always
  // returns a valid holder — canonical tensor_zero on the
  // "no-dependency" path. Check the singleton form explicitly so the
  // rule self-canonicalizes instead of relying on the inner_product
  // simplifier to fold the resulting A^r·0·A^{n-1-r} summands.
  if (!dA.is_valid() || is_same<tensor_zero>(dA)) {
    return;
  }

  // Extract integer exponent via try_int_constant (#284 architectural
  // rule). This recognises scalar_constant, scalar_zero, scalar_one,
  // and scalar_negative(int) singletons — the bare
  // `is_same<scalar_constant>` check that the tensor-arg sibling uses
  // would mis-reject pow(A, 1) (literal-1 → scalar_one singleton) as
  // "non-constant".
  auto int_n = try_int_constant(n_expr);
  if (!int_n) {
    throw not_implemented_error(
        "tensor_differentiation_wrt_scalar: pow with non-constant exponent "
        "(tracked in #287)");
  }
  auto n = static_cast<std::int64_t>(*int_n);

  // Build sum: sum_{r=0}^{n-1} (A^r) * (dA/ds) * (A^{n-1-r})
  // For rank-2 A, matrix multiplication is inner_product on the
  // contraction index. A^r and A^{n-1-r} are also rank-2.
  tensor_holder_t sum;
  for (std::int64_t r = 0; r < n; ++r) {
    auto Ar = pow(A, static_cast<int>(r));
    auto An1r = pow(A, static_cast<int>(n - 1 - r));
    // term = Ar * dA * An1r  via two contractions
    auto term = inner_product(std::move(Ar), sequence{2}, dA, sequence{1});
    term = inner_product(std::move(term), sequence{2}, std::move(An1r),
                         sequence{1});
    if (sum.is_valid()) {
      sum += term;
    } else {
      sum = std::move(term);
    }
  }
  m_result = std::move(sum);
}

// tensor_mul: product rule over the data() vector
// d/ds(A_1 * A_2 * ... * A_n) = sum_j A_1 * ... * A_{j-1} * (dA_j/ds) *
// A_{j+1} * ... * A_n
//
// The j-th factor's scalar derivative dA_j/ds has the SAME rank as
// A_j (scalar arg adds no indices), so the resulting product is
// shape-equivalent to the original tensor_mul. No reorder permutation
// needed.
void tensor_differentiation_wrt_scalar::operator()(
    tensor_mul const &visitable) {
  auto const &factors = visitable.data();
  tensor_holder_t sum;

  for (std::size_t j = 0; j < factors.size(); ++j) {
    auto dAj = diff(factors[j], m_arg);
    // Zero-suppress: skip terms whose derivative is invalid OR the
    // canonical tensor_zero singleton. Without the singleton check,
    // each zero summand would inflate the printed result and confuse
    // hash lock-ins. Matches the tensor_scalar_mul rule's pattern.
    if (!dAj.is_valid() || is_same<tensor_zero>(dAj)) {
      continue;
    }

    // Build lhs chain: factors[0] * ... * factors[j-1]
    tensor_holder_t lhs;
    for (std::size_t i = 0; i < j; ++i) {
      if (!lhs.is_valid()) {
        lhs = factors[i];
      } else {
        auto rank_lhs = lhs.get().rank();
        lhs = inner_product(std::move(lhs), sequence{rank_lhs}, factors[i],
                            sequence{1});
      }
    }

    // Build rhs chain: factors[j+1] * ... * factors[n-1]
    tensor_holder_t rhs;
    for (std::size_t i = j + 1; i < factors.size(); ++i) {
      if (!rhs.is_valid()) {
        rhs = factors[i];
      } else {
        auto rank_rhs = rhs.get().rank();
        rhs = inner_product(std::move(rhs), sequence{rank_rhs}, factors[i],
                            sequence{1});
      }
    }

    // Build term = lhs * dAj * rhs
    tensor_holder_t term;
    if (lhs.is_valid()) {
      auto rank_lhs = lhs.get().rank();
      term =
          inner_product(std::move(lhs), sequence{rank_lhs}, dAj, sequence{1});
    } else {
      term = dAj;
    }

    if (rhs.is_valid()) {
      auto rank_term = term.get().rank();
      term = inner_product(std::move(term), sequence{rank_term}, std::move(rhs),
                           sequence{1});
    }

    if (term.is_valid()) {
      sum += term;
    }
  }

  // Apply coefficient if present AND sum is valid. Guarding on
  // sum.is_valid() avoids constructing a tensor_scalar_mul with an
  // invalid LHS when every factor's derivative was zero.
  if (visitable.coeff().is_valid() && sum.is_valid()) {
    m_result = std::move(sum) * visitable.coeff();
  } else {
    m_result = std::move(sum);
  }
}

// simple_outer_product: d/ds(A_1 ⊗ ... ⊗ A_n) = sum_j A_1 ⊗ ... ⊗
// (dA_j/ds) ⊗ ... ⊗ A_n. dA_j/ds has the same rank as A_j so the
// resulting outer product has the same total rank as the original
// (no permutation reorder needed).
void tensor_differentiation_wrt_scalar::operator()(
    simple_outer_product const &visitable) {
  auto const &factors = visitable.data();
  tensor_holder_t sum;

  for (std::size_t j = 0; j < factors.size(); ++j) {
    auto dAj = diff(factors[j], m_arg);
    // Pass-5 review: same as tensor_pow/inv — !is_valid() is dead
    // post-apply()-fix. Self-canonicalize on zero summands.
    if (!dAj.is_valid() || is_same<tensor_zero>(dAj)) {
      continue;
    }

    // Build lhs chain: factors[0] ⊗ ... ⊗ factors[j-1]
    tensor_holder_t lhs;
    std::size_t rank_lhs = 0;
    for (std::size_t i = 0; i < j; ++i) {
      if (!lhs.is_valid()) {
        lhs = factors[i];
        rank_lhs = factors[i].get().rank();
      } else {
        auto fi_rank = factors[i].get().rank();
        sequence lhs_idx(rank_lhs), rhs_idx(fi_rank);
        std::iota(lhs_idx.begin(), lhs_idx.end(), std::size_t{0});
        std::iota(rhs_idx.begin(), rhs_idx.end(), rank_lhs);
        lhs = otimes(std::move(lhs), std::move(lhs_idx), factors[i],
                     std::move(rhs_idx));
        rank_lhs += fi_rank;
      }
    }

    // Build rhs chain: factors[j+1] ⊗ ... ⊗ factors[n-1]
    tensor_holder_t rhs;
    std::size_t rank_rhs = 0;
    for (std::size_t i = j + 1; i < factors.size(); ++i) {
      if (!rhs.is_valid()) {
        rhs = factors[i];
        rank_rhs = factors[i].get().rank();
      } else {
        auto fi_rank = factors[i].get().rank();
        sequence lhs_idx(rank_rhs), rhs_idx(fi_rank);
        std::iota(lhs_idx.begin(), lhs_idx.end(), std::size_t{0});
        std::iota(rhs_idx.begin(), rhs_idx.end(), rank_rhs);
        rhs = otimes(std::move(rhs), std::move(lhs_idx), factors[i],
                     std::move(rhs_idx));
        rank_rhs += fi_rank;
      }
    }

    // term = lhs ⊗ dAj ⊗ rhs
    tensor_holder_t term;
    std::size_t current_rank = 0;
    if (lhs.is_valid()) {
      auto dAj_rank = dAj.get().rank();
      sequence l_idx(rank_lhs), r_idx(dAj_rank);
      std::iota(l_idx.begin(), l_idx.end(), std::size_t{0});
      std::iota(r_idx.begin(), r_idx.end(), rank_lhs);
      term = otimes(std::move(lhs), std::move(l_idx), dAj, std::move(r_idx));
      current_rank = rank_lhs + dAj_rank;
    } else {
      term = dAj;
      current_rank = dAj.get().rank();
    }

    if (rhs.is_valid()) {
      sequence l_idx(current_rank), r_idx(rank_rhs);
      std::iota(l_idx.begin(), l_idx.end(), std::size_t{0});
      std::iota(r_idx.begin(), r_idx.end(), current_rank);
      term = otimes(std::move(term), std::move(l_idx), std::move(rhs),
                    std::move(r_idx));
    }

    if (term.is_valid()) {
      sum += term;
    }
  }

  m_result = std::move(sum);
}

// tensor_inv: d/ds(A^{-1}) = -A^{-1} * dA/ds * A^{-1}
// Symbolic Magnus formula. Unlike the tensor-arg visitor (which has
// sym/skew kernel variants because differentiating w.r.t. a rank-2
// tensor X expands the rank), the scalar-arg derivative dA/ds has the
// SAME rank as A and the result is again rank(A): the formula is the
// same at any rank, only the contraction layout changes.
//
//   rank 2:  -A^{-1}_{im} · (dA/ds)_{mn} · A^{-1}_{nj}
//            two single-index contractions on (2)↔(1).
//   rank 4:  -A^{-1}_{ijmn} · (dA/ds)_{mnpq} · A^{-1}_{pqkl}
//            two two-index contractions on (3,4)↔(1,2).
//
// Rank-4 is the same Magnus formula in symbolic form regardless of the
// algebraic class (general/minor-sym/skew); the evaluator dispatches
// on annotation at evaluation time via tensor_inv's space tag. So no
// extra branching needed here. Other ranks (3, 5, 6...) keep the
// rank-not-supported throw — `inv()` itself only admits rank 2 and 4,
// so a non-2/4 input here would have been rejected by the factory; the
// throw is a belt-and-braces guard for direct
// `make_expression<tensor_inv>` constructions.
void tensor_differentiation_wrt_scalar::operator()(
    tensor_inv const &visitable) {
  auto const &A = visitable.expr();
  auto dA = diff(A, m_arg);
  // Pass-5 review: see the tensor_pow rule above. Self-canonicalize
  // on the zero-derivative path rather than relying on the
  // inner_product simplifier to fold -(invA · 0 · invA).
  if (!dA.is_valid() || is_same<tensor_zero>(dA)) {
    return;
  }
  // tensor_inv wrapper ctor enforces rank ∈ {2, 4} (#292), so any
  // tensor_inv visited here is guaranteed to be one of those two ranks.
  // Branch explicitly on each — `else` would silently apply the rank-4
  // contraction layout if a future relaxation of the wrapper gate
  // admitted, say, rank-6. std::unreachable() catches that drift in
  // Debug (UB in Release) — same pattern as projector_algebra.h:48.
  auto const r = A.get().rank();
  auto invA = inv(A);
  if (r == 2) {
    // term = invA * dA * invA via two rank-2 contractions
    auto term = inner_product(invA, sequence{2}, std::move(dA), sequence{1});
    term = inner_product(std::move(term), sequence{2}, invA, sequence{1});
    m_result = -std::move(term);
  } else if (r == 4) {
    // rank 4: -invA : dA : invA via two rank-4 double-contractions
    // (#287). The double-contraction `:` is the standard rank-4
    // composition (sequence{3,4} on the LHS pairs with sequence{1,2}
    // on the RHS).
    auto term =
        inner_product(invA, sequence{3, 4}, std::move(dA), sequence{1, 2});
    term = inner_product(std::move(term), sequence{3, 4}, invA, sequence{1, 2});
    m_result = -std::move(term);
  } else {
    std::unreachable();
  }
}

// inner_product_wrapper(A, idxA, B, idxB) — scalar arg product rule:
// d/ds = inner_product(dA, idxA, B, idxB) + inner_product(A, idxA, dB, idxB)
// The contraction indices are unchanged (scalar arg adds no rank).
void tensor_differentiation_wrt_scalar::operator()(
    inner_product_wrapper const &visitable) {
  auto const &expr_lhs = visitable.expr_lhs();
  auto const &expr_rhs = visitable.expr_rhs();
  auto const &seq_lhs = visitable.indices_lhs();
  auto const &seq_rhs = visitable.indices_rhs();

  auto dA = diff(expr_lhs, m_arg);
  auto dB = diff(expr_rhs, m_arg);

  tensor_holder_t sum;
  if (dA.is_valid() && !is_same<tensor_zero>(dA)) {
    auto s_lhs = seq_lhs;
    auto s_rhs = seq_rhs;
    sum = inner_product(std::move(dA), std::move(s_lhs), expr_rhs,
                        std::move(s_rhs));
  }
  if (dB.is_valid() && !is_same<tensor_zero>(dB)) {
    auto s_lhs = seq_lhs;
    auto s_rhs = seq_rhs;
    auto term = inner_product(expr_lhs, std::move(s_lhs), std::move(dB),
                              std::move(s_rhs));
    if (sum.is_valid()) {
      sum += term;
    } else {
      sum = std::move(term);
    }
  }
  m_result = std::move(sum);
}

// outer_product_wrapper(A, idxA, B, idxB) — scalar arg product rule:
// d/ds = otimes(dA, idxA, B, idxB) + otimes(A, idxA, dB, idxB)
// Indices unchanged.
void tensor_differentiation_wrt_scalar::operator()(
    outer_product_wrapper const &visitable) {
  auto const &expr_lhs = visitable.expr_lhs();
  auto const &expr_rhs = visitable.expr_rhs();
  auto const &seq_lhs = visitable.indices_lhs();
  auto const &seq_rhs = visitable.indices_rhs();

  auto dA = diff(expr_lhs, m_arg);
  auto dB = diff(expr_rhs, m_arg);

  tensor_holder_t sum;
  if (dA.is_valid() && !is_same<tensor_zero>(dA)) {
    auto s_lhs = seq_lhs;
    auto s_rhs = seq_rhs;
    sum = otimes(std::move(dA), std::move(s_lhs), expr_rhs, std::move(s_rhs));
  }
  if (dB.is_valid() && !is_same<tensor_zero>(dB)) {
    auto s_lhs = seq_lhs;
    auto s_rhs = seq_rhs;
    auto term =
        otimes(expr_lhs, std::move(s_lhs), std::move(dB), std::move(s_rhs));
    if (sum.is_valid()) {
      sum += term;
    } else {
      sum = std::move(term);
    }
  }
  m_result = std::move(sum);
}

// tensor_to_scalar_with_tensor_mul(T, t2s) — product T * t2s where T
// is tensor and t2s is tensor_to_scalar (result is tensor-valued).
// Full product rule: d/ds(T * t2s) = (dT/ds) * t2s + T * (dt2s/ds).
// Uses #285's diff(t2s, scalar) for the t2s side and the recursive
// diff(tensor, scalar) for the tensor side.
void tensor_differentiation_wrt_scalar::operator()(
    tensor_to_scalar_with_tensor_mul const &visitable) {
  auto const &T = visitable.expr_lhs();
  auto const &s_t2s = visitable.expr_rhs();
  auto dT = diff(T, m_arg);     // tensor (this visitor)
  auto ds = diff(s_t2s, m_arg); // t2s (the #285 visitor)
  tensor_holder_t sum;
  if (dT.is_valid() && !is_same<tensor_zero>(dT)) {
    sum = std::move(dT) * s_t2s;
  }
  if (ds.is_valid() && !is_same<tensor_to_scalar_zero>(ds)) {
    auto term = T * std::move(ds);
    if (sum.is_valid()) {
      sum += term;
    } else {
      sum = std::move(term);
    }
  }
  m_result = std::move(sum);
}

// Top-level CPO definition (declared in the header).
expression_holder<tensor_expression>
tag_invoke(detail::diff_fn, std::type_identity<tensor_expression>,
           std::type_identity<scalar_expression>,
           expression_holder<tensor_expression> const &expr,
           expression_holder<scalar_expression> const &arg) {
  tensor_differentiation_wrt_scalar d(arg);
  return d.apply(expr);
}

} // namespace numsim::cas
