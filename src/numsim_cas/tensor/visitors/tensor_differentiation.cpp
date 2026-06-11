#include <numsim_cas/tensor/visitors/tensor_differentiation.h>

#include <numeric>
#include <numsim_cas/core/diff.h>
#include <numsim_cas/scalar/scalar_functions.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor/tensor_assume.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_diff.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>

namespace numsim::cas {

// tensor_pow: d(A^n)/dX = sum_{r=0}^{n-1} T_r : dA/dX
// where T_r[i,j,p,q] = (A^r)_{ip} * (A^{n-1-r})_{qj}  (Daleckij-Krein)
void tensor_differentiation::operator()(tensor_pow const &visitable) {
  auto const &A = visitable.expr_lhs();
  auto const &n_expr = visitable.expr_rhs();

  auto dA = diff(A, m_arg);
  if (!dA.is_valid()) {
    return;
  }

  // Extract integer exponent via try_int_constant (#284 architectural
  // rule). The bare `is_same<scalar_constant>(n_expr)` check this
  // replaced mishandled pow(A, 1) and pow(A, 0) because the parser /
  // factory uses scalar_one / scalar_zero singletons for those values
  // — not scalar_constant{1/0}. Result: `diff(pow(A, 1), X)` threw
  // not_implemented_error even though the math is trivial (it's just
  // diff(A, X)). try_int_constant correctly recognises the singletons
  // and the scalar_negative(scalar_constant) form (#284 audit).
  auto int_n = try_int_constant(n_expr);
  if (!int_n) {
    throw not_implemented_error(
        "tensor_differentiation: pow with non-constant exponent");
  }
  auto n = static_cast<std::int64_t>(*int_n);

  // n == 0: pow(A, 0) = I (constant), derivative is zero. The loop
  // below would correctly leave sum invalid and apply()'s fallback
  // would coerce to tensor_zero, but an explicit guard documents the
  // intent and avoids confusing future readers about what an invalid
  // sum means here. Per #284a / PR #288 review feedback.
  if (n == 0) {
    m_result = make_expression<tensor_zero>(m_dim, m_rank_result);
    return;
  }

  // Build sum: sum_{r=0}^{n-1} inner_product(T_r, {3,4}, dA/dX, {1,2})
  // T_r = otimes(A^r, {1,3}, A^{n-1-r}, {4,2})
  //   so T_r[i,j,p,q] = (A^r)_{ip} * (A^{n-1-r})_{qj}
  tensor_holder_t sum;
  for (std::int64_t r = 0; r < n; ++r) {
    auto Ar = pow(A, static_cast<int>(r));
    auto An1r = pow(A, static_cast<int>(n - 1 - r));
    auto T = otimes(Ar, sequence{1, 3}, An1r, sequence{4, 2});
    auto term = inner_product(T, sequence{3, 4}, dA, sequence{1, 2});
    if (sum.is_valid()) {
      sum += term;
    } else {
      sum = std::move(term);
    }
  }

  m_result = std::move(sum);
}

// tensor_mul: product rule over the data() vector
// d(A1*A2*...*An)/dX = sum_j coeff * lhs * dAj * rhs
// where lhs = A0*...*A_{j-1}, rhs = A_{j+1}*...*A_{n-1}
void tensor_differentiation::operator()(tensor_mul const &visitable) {
  auto const &factors = visitable.data();
  tensor_holder_t sum;

  for (std::size_t j = 0; j < factors.size(); ++j) {
    auto dAj = diff(factors[j], m_arg);
    if (!dAj.is_valid()) {
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
      // The exit index (last original index of factor j) is at 1-based
      // position exit_pos = term.rank() - m_rank_arg, with derivative
      // indices appended after it.
      auto exit_pos = term.get().rank() - m_rank_arg; // 1-based
      auto rank_rhs = rhs.get().rank();

      // Contract at the exit position
      term = inner_product(std::move(term), sequence{exit_pos}, std::move(rhs),
                           sequence{1});

      // Permute [L][D][R] → [L][R][D] to move derivative indices to end
      // permute_indices convention: output(args) = input(args[perm[0]], ...)
      // perm[k] maps input position k → output position that feeds into it
      auto total = term.get().rank();
      auto R = rank_rhs - 1;
      auto L = total - m_rank_arg - R;
      if (R > 0) {
        sequence reorder(total);
        std::iota(reorder.begin(),
                  reorder.begin() + static_cast<std::ptrdiff_t>(L),
                  std::size_t{0});
        std::iota(reorder.begin() + static_cast<std::ptrdiff_t>(L),
                  reorder.begin() + static_cast<std::ptrdiff_t>(L + m_rank_arg),
                  L + R);
        std::iota(reorder.begin() + static_cast<std::ptrdiff_t>(L + m_rank_arg),
                  reorder.end(), L);
        term = permute_indices(std::move(term), std::move(reorder));
      }
    }

    if (term.is_valid()) {
      sum += term;
    }
  }

  // Apply coefficient if present
  if (visitable.coeff().is_valid()) {
    m_result = std::move(sum) * visitable.coeff();
  } else {
    m_result = std::move(sum);
  }
}

// simple_outer_product: product rule (outer products, no contraction)
// d(A1⊗A2⊗...⊗An)/dX = sum_j lhs ⊗ dAj ⊗ rhs
// where lhs = A0⊗...⊗A_{j-1}, rhs = A_{j+1}⊗...⊗A_{n-1}
void tensor_differentiation::operator()(simple_outer_product const &visitable) {
  auto const &factors = visitable.data();
  tensor_holder_t sum;

  for (std::size_t j = 0; j < factors.size(); ++j) {
    auto dAj = diff(factors[j], m_arg);
    if (!dAj.is_valid()) {
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

    // Build term = lhs ⊗ dAj ⊗ rhs
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

      // Permute: move derivative indices from middle to end
      // Layout is [lhs][factor_j_orig][D][rhs], desired
      // [lhs][factor_j_orig][rhs][D] permute_indices convention: perm[k] maps
      // input pos k → output pos
      auto L = rank_lhs + factors[j].get().rank();
      auto total = current_rank + rank_rhs;
      sequence reorder(total);
      std::iota(reorder.begin(),
                reorder.begin() + static_cast<std::ptrdiff_t>(L),
                std::size_t{0});
      std::iota(reorder.begin() + static_cast<std::ptrdiff_t>(L),
                reorder.begin() + static_cast<std::ptrdiff_t>(L + m_rank_arg),
                L + rank_rhs);
      std::iota(reorder.begin() + static_cast<std::ptrdiff_t>(L + m_rank_arg),
                reorder.end(), L);
      term = permute_indices(std::move(term), std::move(reorder));
    }

    if (term.is_valid()) {
      sum += term;
    }
  }

  m_result = std::move(sum);
}

// tensor_inv: d(A^{-1})/dX = -T : dA/dX, with kernel T chosen by the
// input's rank AND its algebraic class. Six dispatch paths (#250):
//
// === Rank 2 ===
//   - General (no Sym/Skew annotation):
//       T_{ijmn} = A^{-1}_{im} * A^{-1}_{nj}                  (Magnus)
//   - Symmetric (A = A^T):
//       T_{ijmn} = (1/2)(A^{-1}_{im} A^{-1}_{nj}
//                      + A^{-1}_{in} A^{-1}_{mj})             (minor-sym)
//   - Skew (A = -A^T, even dim only — odd-dim skew is singular and is
//     already rejected by the inv() factory):
//       T_{ijmn} = (1/2)(A^{-1}_{im} A^{-1}_{nj}
//                      - A^{-1}_{in} A^{-1}_{mj})             (minor-antisym)
//
// === Rank 4 (#250 rank-4) ===
//   - General (no Minor/MinorMajor annotation):
//       T_{ijkl,mnpq} = A^{-1}_{ijmn} · A^{-1}_{pqkl}         (Magnus)
//   - Minor symmetry on the rank-4 differentiation pair (C_{ijkl} =
//     C_{jikl} = C_{ijlk}): symmetrize T over the 4-element group
//     {id, swap(m,n), swap(p,q), swap both} → 1/4 prefactor.
//   - MinorMajor symmetry (Minor + major-pair swap (mn ↔ pq)):
//     symmetrize over the 8-element group → 1/8 prefactor. Canonical
//     case in continuum-mechanics elasticity tangents.
//
// The Sym/Skew kernels (rank 2) and Minor/MinorMajor kernels (rank 4)
// are NOT contraction tricks over Magnus — they are the unique correct
// derivatives on the symmetric / skew sub-manifold (the Magnus
// formula's projection onto the constrained subspace). For a symmetric
// A, the Magnus kernel and the sym kernel happen to contract to the
// same value when dA is also symmetric (swap-index proof), so callers
// who feed sym dA in won't observe a numerical difference — but the
// AST is now structurally honest, and callers who pass an un-symmetrized
// dA against a symmetric input get the mathematically correct answer
// instead of silently-wrong. Same property holds for rank-4 minor /
// minor-major dispatch.
//
// Dispatch goes through is_symmetric() / is_skew() / is_minor() /
// is_minor_major() (tensor_assume.h), not raw holds_alternative on
// perm. This picks up the PD/PSD ⇒ symmetric implication from the
// algebra-assumption manager (rank-2), including the case where
// space() is cleared but the algebra tag remains. Vol/Dev inputs route
// through the sym branch too (their space()->perm is Symmetric{}).
// Closes #250.
void tensor_differentiation::operator()(tensor_inv const &visitable) {
  auto const &A = visitable.expr();
  auto dA = diff(A, m_arg);
  if (!dA.is_valid()) {
    return;
  }

  auto invA = inv(A);

  if (A.get().rank() == 2) {
    auto T = otimes(invA, sequence{1, 3}, invA, sequence{4, 2});
    // Pick sym/skew kernel sign; sign==0 means general (Magnus) — no swap term.
    int sign = 0;
    if (is_symmetric(A))
      sign = +1;
    else if (is_skew(A)) {
      // Cross-module precondition: the inv() factory rejects odd-dim
      // skew (singular) at tensor_functions.h:425+. A direct
      // `make_expression<tensor_inv>(odd_dim_skew)` bypasses the factory,
      // and a future relaxation of the factory check would silently emit
      // a meaningless antisym kernel for a singular operand. Throw
      // matches the factory's `invalid_expression_error` convention and
      // — unlike `assert` — fires in Release builds too.
      if (A.get().dim() % 2 != 0)
        throw invalid_expression_error(
            "tensor_differentiation: odd-dim skew inv-diff is undefined "
            "(singular matrix); should have been rejected at the inv() "
            "factory");
      sign = -1;
    }
    if (sign != 0) {
      auto T_swap = otimes(invA, sequence{1, 4}, invA, sequence{3, 2});
      // The 1/2 constant: function-local static. Magic-static
      // initialization is thread-safe per C++17 [stmt.dcl]; the lazy
      // hash computation on the shared instance (mutable m_hash_value
      // in expression_base) is NOT synchronized — see the note in
      // include/numsim_cas/core/expression.h. The codebase is
      // single-threaded today so this is fine; if multithreading lands,
      // this static needs to participate in whatever synchronization
      // scheme the hash cache adopts.
      //
      // PEER-SITE NOTE: scalar_constant(scalar_number{1, 2}) is also
      // inlined per-call at scalar_simplifier_pow.cpp:95, scalar_std.h:
      // 208 and :233. Those sites are construction-time and not on a
      // hot path; the diff visitor IS called per tensor_inv per
      // derivative, hence the local static here. DO NOT unify the four
      // sites into a `get_scalar_half()` in scalar_globals.h — the
      // existing get_scalar_zero / get_scalar_one return dedicated
      // SINGLETON NODE TYPES (scalar_zero / scalar_one), not constants;
      // promoting a rational-constant to the same API is a category
      // mismatch (intentionally rolled back in commit ad4d824).
      static auto const half =
          make_expression<scalar_constant>(scalar_number{1, 2});
      T = (sign > 0 ? T + T_swap : T - T_swap) * half;
    }
    m_result = -inner_product(T, sequence{3, 4}, dA, sequence{1, 2});
  } else if (A.get().rank() == 4) {
    // Rank-4 inv-diff. Output index convention (positions 1..8) =
    // (i, j, k, l, m, n, p, q) where (i,j,k,l) are the free indices of
    // A^{-1} and (m,n,p,q) are the indices that pair with dA's
    // A-indices for the chain-rule contraction.
    //
    // General (Magnus) kernel from #250:
    //   T_{ijkl,mnpq} = invA_{ijmn} · invA_{pqkl}
    // built via otimes with first invA at output positions (1,2,5,6)
    // and second invA at output positions (7,8,3,4).
    //
    // Annotation-dependent symmetrizers (analogous to rank-2 sym/skew):
    //   - is_minor(A):       symmetrize over the 4-element group
    //                        {id, swap(m,n), swap(p,q), swap both} → 1/4
    //   - is_minor_major(A): symmetrize over the 8-element group
    //                        Minor ∪ (Minor ∘ major-swap(mn↔pq)) → 1/8
    //   - else:              general Magnus, no symmetrization
    //
    // A^{-1}'s output free indices (i,j,k,l) inherit MinorMajor symmetry
    // automatically from A's annotation (the tensor_inv wrapper
    // propagates Minor/MinorMajor to the result), so S_out is implicit
    // and we only need to explicitly apply S_in to the (m,n,p,q) side.
    // See #250's "Compact form via the projector framework".
    auto T = otimes(invA, sequence{1, 2, 5, 6}, invA, sequence{7, 8, 3, 4});

    // Dispatch on annotation. `is_minor_major` implies `is_minor`
    // algebraically (MinorMajor ⊃ Minor), but `is_minor()` checks the
    // perm tag for `Minor` specifically — not `MinorMajor`. Keep the
    // explicit OR so dispatch survives a future predicate refactor
    // (e.g. if `is_minor` becomes "exactly Minor"). Same defensive
    // pattern as the rank-2 sign dispatch above.
    const bool minor = is_minor(A) || is_minor_major(A);
    const bool major = is_minor_major(A);

    if (minor) {
      auto T_swap_mn =
          otimes(invA, sequence{1, 2, 6, 5}, invA, sequence{7, 8, 3, 4});
      auto T_swap_pq =
          otimes(invA, sequence{1, 2, 5, 6}, invA, sequence{8, 7, 3, 4});
      auto T_swap_both =
          otimes(invA, sequence{1, 2, 6, 5}, invA, sequence{8, 7, 3, 4});

      if (major) {
        // Major-swap variants: swap (m,n) ↔ (p,q) in the parameterization.
        // Pattern: first invA gets (p,q) at its trailing pair, second
        // invA gets (m,n) at its leading pair.
        auto T_M =
            otimes(invA, sequence{1, 2, 7, 8}, invA, sequence{5, 6, 3, 4});
        auto T_M_mn =
            otimes(invA, sequence{1, 2, 7, 8}, invA, sequence{6, 5, 3, 4});
        auto T_M_pq =
            otimes(invA, sequence{1, 2, 8, 7}, invA, sequence{5, 6, 3, 4});
        auto T_M_both =
            otimes(invA, sequence{1, 2, 8, 7}, invA, sequence{6, 5, 3, 4});
        // Same magic-static rationale as the rank-2 `half` constant above.
        static auto const eighth =
            make_expression<scalar_constant>(scalar_number{1, 8});
        T = (T + T_swap_mn + T_swap_pq + T_swap_both + T_M + T_M_mn + T_M_pq +
             T_M_both) *
            eighth;
      } else {
        static auto const fourth =
            make_expression<scalar_constant>(scalar_number{1, 4});
        T = (T + T_swap_mn + T_swap_pq + T_swap_both) * fourth;
      }
    }

    // Contract T's positions (5,6,7,8) = (m,n,p,q) with dA's A-indices
    // (positions 1..4). dA has rank 4 + rank(m_arg); the result's free
    // rank is 4 + rank(m_arg). Concretely:
    //   X rank-2 → dA rank-6 → result rank-6
    //   X rank-4 → dA rank-8 → result rank-8
    m_result =
        -inner_product(T, sequence{5, 6, 7, 8}, dA, sequence{1, 2, 3, 4});
  } else {
    // Wrapper ctor (#292) gates rank ∈ {2, 4}; this branch is
    // structurally unreachable for tensor_inv constructed via the
    // factory or make_expression. Kept as a defensive guard.
    //
    // Policy split with the scalar-arg sibling (#287): that visitor at
    // tensor_differentiation_wrt_scalar.cpp uses `std::unreachable()`
    // for the same condition. Here we throw because the diagnostic
    // value at the runtime boundary (clear Release error vs UB) is
    // worth the redundancy; the scalar-arg path inherits the rank-2
    // sym/skew structure and has a single explicit if/else if/else.
    throw not_implemented_error(
        "tensor_differentiation: inv derivative for rank != 2 and != 4 "
        "is impossible — tensor_inv wrapper rejects other ranks at "
        "construction (#292)");
  }
}

// inner_product_wrapper: product rule
// d(inner(A, idx_a, B, idx_b))/dX = inner(dA, idx_a, B, idx_b) + inner(A,
// idx_a, dB, idx_b) Derivative indices are appended, so contraction positions
// are unchanged.
void tensor_differentiation::operator()(
    inner_product_wrapper const &visitable) {
  auto const &expr_lhs = visitable.expr_lhs();
  auto const &expr_rhs = visitable.expr_rhs();
  auto const &seq_lhs = visitable.indices_lhs();
  auto const &seq_rhs = visitable.indices_rhs();

  auto dA = diff(expr_lhs, m_arg);
  auto dB = diff(expr_rhs, m_arg);

  const auto rank_lhs = expr_lhs.get().rank();
  const auto rank_rhs = expr_rhs.get().rank();
  const auto size_inner_lhs = seq_lhs.size();
  const auto free_lhs = rank_lhs - size_inner_lhs;
  const auto free_rhs = rank_rhs - seq_rhs.size();

  tensor_holder_t result;

  if (dA.is_valid() && !is_same<tensor_zero>(dA)) {
    // dA has rank = rank_lhs + rank_arg, derivative indices appended.
    // Contraction indices are at the same positions as in the original A.
    auto term_lhs =
        inner_product(dA, sequence(seq_lhs), expr_rhs, sequence(seq_rhs));

    if (free_rhs > 0) {
      // Need to reorder: put derivative indices (from arg) at the end
      // Original result: free_lhs indices, rank_arg indices, free_rhs indices
      // Desired: free_lhs indices, free_rhs indices, rank_arg indices
      // permute_indices convention: perm[k] maps input pos k → output pos
      std::size_t new_rank = free_lhs + m_rank_arg + free_rhs;
      sequence reorder(new_rank);
      // L block (free_lhs): input 0..L-1 ← output 0..L-1
      std::iota(reorder.begin(),
                reorder.begin() + static_cast<std::ptrdiff_t>(free_lhs),
                std::size_t{0});
      // D block (rank_arg): input L..L+D-1 ← output L+R..L+R+D-1
      std::iota(reorder.begin() + static_cast<std::ptrdiff_t>(free_lhs),
                reorder.begin() +
                    static_cast<std::ptrdiff_t>(free_lhs + m_rank_arg),
                free_lhs + free_rhs);
      // R block (free_rhs): input L+D..L+D+R-1 ← output L..L+R-1
      std::iota(reorder.begin() +
                    static_cast<std::ptrdiff_t>(free_lhs + m_rank_arg),
                reorder.end(), free_lhs);
      term_lhs = permute_indices(std::move(term_lhs), std::move(reorder));
    }

    result = std::move(term_lhs);
  }

  if (dB.is_valid() && !is_same<tensor_zero>(dB)) {
    // dB has rank = rank_rhs + rank_arg, derivative indices appended.
    // Contraction indices are at the same positions as in the original B.
    auto term_rhs =
        inner_product(expr_lhs, sequence(seq_lhs), dB, sequence(seq_rhs));

    if (result.is_valid()) {
      result += term_rhs;
    } else {
      result = std::move(term_rhs);
    }
  }

  m_result = std::move(result);
}

// permute_indices_wrapper: d(permute(A, indices))/dX = permute(dA,
// extended_indices)
void tensor_differentiation::operator()(
    permute_indices_wrapper const &visitable) {
  auto const &child = visitable.expr();
  auto const &indices = visitable.indices();

  auto dA = diff(child, m_arg);
  if (!dA.is_valid()) {
    return;
  }

  // Extend the permutation: original indices stay, append identity for arg
  // indices
  sequence extended(indices.size() + m_rank_arg);
  for (std::size_t i = 0; i < indices.size(); ++i) {
    extended[i] = indices[i];
  }
  std::iota(extended.begin() + static_cast<std::ptrdiff_t>(indices.size()),
            extended.end(), indices.size());

  m_result = permute_indices(std::move(dA), std::move(extended));
}

// outer_product_wrapper: product rule with outer products
void tensor_differentiation::operator()(
    outer_product_wrapper const &visitable) {
  auto const &expr_lhs = visitable.expr_lhs();
  auto const &expr_rhs = visitable.expr_rhs();
  auto const &seq_lhs = visitable.indices_lhs();
  auto const &seq_rhs = visitable.indices_rhs();

  auto dA = diff(expr_lhs, m_arg);
  auto dB = diff(expr_rhs, m_arg);

  const auto rank_lhs = expr_lhs.get().rank();
  const auto rank_rhs = expr_rhs.get().rank();

  tensor_holder_t result;

  if (dA.is_valid()) {
    // dA has extra rank_arg indices at the end
    // New lhs indices: original seq_lhs positions
    sequence new_lhs_idx(seq_lhs.size() + m_rank_arg);
    for (std::size_t i = 0; i < seq_lhs.size(); ++i) {
      new_lhs_idx[i] = seq_lhs[i];
    }
    // Append derivative indices
    std::iota(new_lhs_idx.begin() + static_cast<std::ptrdiff_t>(seq_lhs.size()),
              new_lhs_idx.end(), rank_lhs + rank_rhs);
    result = otimes(std::move(dA), std::move(new_lhs_idx), expr_rhs,
                    sequence(seq_rhs));
  }

  if (dB.is_valid()) {
    // dB has extra rank_arg indices at the end
    sequence new_rhs_idx(seq_rhs.size() + m_rank_arg);
    for (std::size_t i = 0; i < seq_rhs.size(); ++i) {
      new_rhs_idx[i] = seq_rhs[i];
    }
    std::iota(new_rhs_idx.begin() + static_cast<std::ptrdiff_t>(seq_rhs.size()),
              new_rhs_idx.end(), rank_lhs + rank_rhs);
    auto term = otimes(expr_lhs, sequence(seq_lhs), std::move(dB),
                       std::move(new_rhs_idx));
    if (result.is_valid()) {
      result += term;
    } else {
      result = std::move(term);
    }
  }

  m_result = std::move(result);
}

// tensor_to_scalar_with_tensor_mul: (f*A)' = f*dA + otimes(A, df)
// where f is tensor_to_scalar (expr_rhs is t2s, expr_lhs is tensor)
// Actually from the class definition:
// binary_op<..., tensor_expression, tensor_to_scalar_expression>
// so expr_lhs() = tensor, expr_rhs() = t2s
void tensor_differentiation::operator()(
    tensor_to_scalar_with_tensor_mul const &visitable) {
  auto const &A = visitable.expr_lhs(); // tensor
  auto const &f = visitable.expr_rhs(); // tensor_to_scalar

  auto dA = diff(A, m_arg);
  auto df = diff(f, m_arg); // returns tensor (cross-domain)

  tensor_holder_t result;

  if (dA.is_valid()) {
    result = make_expression<tensor_to_scalar_with_tensor_mul>(dA, f);
  }

  if (df.is_valid()) {
    auto outer_term = otimes(A, df);
    if (result.is_valid()) {
      result += outer_term;
    } else {
      result = std::move(outer_term);
    }
  }

  m_result = std::move(result);
}

} // namespace numsim::cas
