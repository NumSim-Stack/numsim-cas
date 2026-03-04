#include <numsim_cas/tensor/visitors/tensor_differentiation.h>

#include <numeric>
#include <numsim_cas/core/diff.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_diff.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>

namespace numsim::cas {

// tensor_pow: d(A^n)/dX = sum_{r=0}^{n-1} otimesu(A^r, A^{n-1-r}) : dA/dX
void tensor_differentiation::operator()(tensor_pow const &visitable) {
  auto const &A = visitable.expr_lhs();
  auto const &n_expr = visitable.expr_rhs();

  auto dA = diff(A, m_arg);
  if (!dA.is_valid()) {
    return;
  }

  // Extract integer exponent
  if (!is_same<scalar_constant>(n_expr)) {
    throw not_implemented_error(
        "tensor_differentiation: pow with non-constant exponent");
  }
  auto const &val = n_expr.template get<scalar_constant>().value();
  auto n = std::get<std::int64_t>(val.raw());

  // Build sum: sum_{r=0}^{n-1} inner_product(otimesu(A^r, A^{n-1-r}), {3,4},
  // dA/dX, {1,2})
  tensor_holder_t sum;
  for (std::int64_t r = 0; r < n; ++r) {
    auto Ar = pow(A, static_cast<int>(r));
    auto An1r = pow(A, static_cast<int>(n - 1 - r));
    auto term =
        inner_product(otimesu(Ar, An1r), sequence{3, 4}, dA, sequence{1, 2});
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
      // basis_change convention: output(args) = input(args[perm[0]], ...)
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
      // [lhs][factor_j_orig][rhs][D] basis_change convention: perm[k] maps
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

// tensor_inv: d(A^{-1})/dX = -inner(inner(inv(A), dA/dX), inv(A))
// For rank-2: d(A^{-1})_{ij}/dX_{kl} = -A^{-1}_{im} (dA_{mn}/dX_{kl})
// A^{-1}_{nj}
void tensor_differentiation::operator()(tensor_inv const &visitable) {
  auto const &A = visitable.expr();
  auto dA = diff(A, m_arg);
  if (!dA.is_valid()) {
    return;
  }

  auto invA = inv(A);

  if (A.get().rank() == 2) {
    // d(inv(A))/dX = -inv(A)_{im} * dA_{mn,kl} * inv(A)_{nj}
    // = -inner(inv(A), {2}, dA, {1}) contracted with inv(A) on remaining
    auto temp = inner_product(invA, sequence{2}, dA, sequence{1});
    m_result = -inner_product(std::move(temp), sequence{3}, invA, sequence{1});
  } else {
    throw not_implemented_error(
        "tensor_differentiation: inv derivative for rank != 2");
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
      // basis_change convention: perm[k] maps input pos k → output pos
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

// basis_change_imp: d(permute(A, indices))/dX = permute(dA, extended_indices)
void tensor_differentiation::operator()(basis_change_imp const &visitable) {
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
