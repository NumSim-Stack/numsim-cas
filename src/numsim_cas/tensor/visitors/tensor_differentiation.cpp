#include <numsim_cas/tensor/visitors/tensor_differentiation.h>

#include <numsim_cas/core/diff.h>
#include <numsim_cas/tensor/tensor_diff.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_diff.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor/tensor_std.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>
#include <numeric>

namespace numsim::cas {

// tensor_power_diff: d(A^n)/dX = sum_{r=0}^{n-1} otimesu(A^r, A^{n-1-r}) : dA/dX
void tensor_differentiation::operator()(
    tensor_power_diff const &visitable) {
  auto const &A = visitable.expr_lhs();
  auto const &n_expr = visitable.expr_rhs();

  auto dA = diff(A, m_arg);
  if (!dA.is_valid()) {
    return;
  }

  // The tensor_power_diff node stores (A, n) where n is a scalar expression
  // We expand: sum_{r=0}^{n-1} otimesu(A^r, A^{n-1-r}) contracted with dA/dX
  // For now, create the symbolic expression:
  // inner_product(sum_{r} otimesu(pow(A,r), pow(A,n-1-r)), {3,4}, dA, {1,2})
  auto one = get_scalar_one();
  auto n_minus_one = n_expr - one;

  // Build the sum of otimesu terms
  // For a general symbolic n, we can't expand. But tensor_pow typically has
  // integer exponents. We'll build the symbolic chain rule form:
  // d(A^n)/dX = n * inner(otimesu(A^0, A^{n-1}) + ... ), dA)
  // Simplified: just return the tensor_power_diff node as-is for symbolic form
  // The evaluator will handle the expansion.
  // However, we need to contract with dA/dX:
  m_result = inner_product(m_expr, sequence{3, 4}, dA, sequence{1, 2});
}

// tensor_mul: product rule over the data() vector
// d(A1*A2*...*An)/dX = sum_j (prod_{i!=j} Ai) * dAj/dX
void tensor_differentiation::operator()(tensor_mul const &visitable) {
  auto const &factors = visitable.data();
  tensor_holder_t sum;

  for (std::size_t j = 0; j < factors.size(); ++j) {
    auto dAj = diff(factors[j], m_arg);
    if (!dAj.is_valid()) {
      continue;
    }

    // Build product of all factors except j, contracted with dAj
    // For tensor_mul, each factor is contracted on adjacent indices
    // The derivative replaces factor j with dAj (which has extra indices)
    tensor_holder_t term = dAj;
    for (std::size_t i = 0; i < factors.size(); ++i) {
      if (i == j) continue;
      // Contract: inner_product on the adjacent indices
      auto rank_term = term.get().rank();
      [[maybe_unused]] auto rank_fi = factors[i].get().rank();
      // Contract last index of term with first index of factor[i]
      // (or first of factor[i] with last of previous result)
      term = inner_product(std::move(term), sequence{rank_term},
                           factors[i], sequence{1});
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
void tensor_differentiation::operator()(
    simple_outer_product const &visitable) {
  auto const &factors = visitable.data();
  tensor_holder_t sum;

  for (std::size_t j = 0; j < factors.size(); ++j) {
    auto dAj = diff(factors[j], m_arg);
    if (!dAj.is_valid()) {
      continue;
    }

    // Build outer product of all factors, replacing factor j with dAj
    tensor_holder_t term = (j == 0) ? dAj : factors[0];
    std::size_t current_rank = term.get().rank();

    for (std::size_t i = 1; i < factors.size(); ++i) {
      auto const &fi = (i == j) ? dAj : factors[i];
      sequence lhs_idx(current_rank), rhs_idx(fi.get().rank());
      std::iota(lhs_idx.begin(), lhs_idx.end(), std::size_t{0});
      std::iota(rhs_idx.begin(), rhs_idx.end(), current_rank);
      term = otimes(std::move(term), std::move(lhs_idx),
                    fi, std::move(rhs_idx));
      current_rank += fi.get().rank();
    }

    if (term.is_valid()) {
      sum += term;
    }
  }

  m_result = std::move(sum);
}

// tensor_inv: d(A^{-1})/dX = -inner(inner(inv(A), dA/dX), inv(A))
// For rank-2: d(A^{-1})_{ij}/dX_{kl} = -A^{-1}_{im} (dA_{mn}/dX_{kl}) A^{-1}_{nj}
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
// d(inner(A, idx_a, B, idx_b))/dX = inner(dA, idx_a, B, idx_b) + inner(A, idx_a, dB, idx_b)
// Derivative indices are appended, so contraction positions are unchanged.
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
    auto term_lhs = inner_product(dA, sequence(seq_lhs),
                                  expr_rhs, sequence(seq_rhs));

    if (free_rhs > 0) {
      // Need to reorder: put derivative indices (from arg) at the end
      // Original result: free_lhs indices, rank_arg indices, free_rhs indices
      // Desired: free_lhs indices, free_rhs indices, rank_arg indices
      std::size_t new_rank = free_lhs + m_rank_arg + free_rhs;
      sequence reorder(new_rank);
      // free_lhs slots: 0..free_lhs-1
      std::iota(reorder.begin(), reorder.begin() + free_lhs, std::size_t{0});
      // free_rhs slots
      std::iota(reorder.begin() + free_lhs,
                reorder.begin() + free_lhs + free_rhs,
                free_lhs + m_rank_arg);
      // rank_arg slots at end
      std::iota(reorder.begin() + free_lhs + free_rhs, reorder.end(),
                free_lhs);
      term_lhs = permute_indices(std::move(term_lhs), std::move(reorder));
    }

    result = std::move(term_lhs);
  }

  if (dB.is_valid() && !is_same<tensor_zero>(dB)) {
    // dB has rank = rank_rhs + rank_arg, derivative indices appended.
    // Contraction indices are at the same positions as in the original B.
    auto term_rhs = inner_product(expr_lhs, sequence(seq_lhs),
                                  dB, sequence(seq_rhs));
    if (result.is_valid()) {
      result += term_rhs;
    } else {
      result = std::move(term_rhs);
    }
  }

  m_result = std::move(result);
}

// basis_change_imp: d(permute(A, indices))/dX = permute(dA, extended_indices)
void tensor_differentiation::operator()(
    basis_change_imp const &visitable) {
  auto const &child = visitable.expr();
  auto const &indices = visitable.indices();

  auto dA = diff(child, m_arg);
  if (!dA.is_valid()) {
    return;
  }

  // Extend the permutation: original indices stay, append identity for arg indices
  sequence extended(indices.size() + m_rank_arg);
  for (std::size_t i = 0; i < indices.size(); ++i) {
    extended[i] = indices[i];
  }
  std::iota(extended.begin() + indices.size(), extended.end(),
            indices.size());

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
    std::iota(new_lhs_idx.begin() + seq_lhs.size(), new_lhs_idx.end(),
              rank_lhs + rank_rhs);
    result = otimes(std::move(dA), std::move(new_lhs_idx),
                    expr_rhs, sequence(seq_rhs));
  }

  if (dB.is_valid()) {
    // dB has extra rank_arg indices at the end
    sequence new_rhs_idx(seq_rhs.size() + m_rank_arg);
    for (std::size_t i = 0; i < seq_rhs.size(); ++i) {
      new_rhs_idx[i] = seq_rhs[i];
    }
    std::iota(new_rhs_idx.begin() + seq_rhs.size(), new_rhs_idx.end(),
              rank_lhs + rank_rhs);
    auto term = otimes(expr_lhs, sequence(seq_lhs),
                       std::move(dB), std::move(new_rhs_idx));
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
  auto const &A = visitable.expr_lhs();   // tensor
  auto const &f = visitable.expr_rhs();   // tensor_to_scalar

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
