#ifndef IDENTITY_TENSOR_H
#define IDENTITY_TENSOR_H

#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

/**
 * @class identity_tensor
 * @brief The symbolic identity (or "minor identity") tensor at any even rank.
 *
 * Represents the rank-2R identity tensor parameterised by spatial dimension
 * `dim` and rank `R`. Only even ranks are meaningful — odd ranks have no
 * consistent minor-identity definition and `tensor_data_identity` rejects
 * them at evaluation time.
 *
 * ## Definition by rank
 *
 * - **Rank 2** — the Kronecker delta: `I_{ij} = δ_{ij}`.
 *   Prints simply as `"I"`. Earlier versions of the codebase had a separate
 *   `kronecker_delta` node for this case; it was removed in favour of this
 *   unified node (see #188). The trivial rank-2 form is also what most
 *   user-facing code sees: `pow(A, 0)`, `dev(I)`, `inv(I)`, `trace(I)`, and
 *   all of the construction-time simplifier rules use the rank-2 form.
 *
 * - **Rank 2R** with `R ≥ 2` — the *minor identity*:
 *
 *     I_{i₁…i_R, j₁…j_R} = ∏_{k=1..R} δ_{i_k j_k}
 *
 *   Concretely at rank 4: `I_{ijkl} = δ_{ik} · δ_{jl}`. The rank-`{2R}`
 *   identity prints as `"I{2R}"` (e.g. `"I{4}"`) to disambiguate it from
 *   the outer-product-style identity discussed below. This is the chain-rule
 *   kernel for tensor self-differentiation:
 *
 *     ∂A_{ij} / ∂A_{kl} = δ_{ik} · δ_{jl} = I_{ijkl}
 *
 *   `diff(A, A)` for a rank-2 tensor `A` produces this rank-4 minor identity.
 *
 * ## Why not just `tmech::eye<T, D, R>`?
 *
 * `tmech::eye<T, D, 4>` is the *outer-product* identity:
 *
 *     eye<T,D,4>_{ijkl} = δ_{ij} · δ_{kl}        // outer-product pairing
 *
 * which is the wrong pairing for differentiation — using it for
 * `∂A_{ij}/∂A_{kl}` would compute `δ_{ij}·δ_{kl}` instead of `δ_{ik}·δ_{jl}`.
 * The evaluator therefore builds the rank-4 minor identity explicitly as
 * `tmech::otimesu(I2, I2)` rather than `tmech::eye<T, D, 4>`. See
 * `include/numsim_cas/tensor/data/tensor_data_unary_wrapper.h::evaluate_imp`
 * for the runtime construction (rank 2 uses `tmech::eye`, rank 4 uses
 * `tmech::otimesu(I,I)`, general rank-2R uses an explicit flat-index loop
 * over the minor-identity product).
 *
 * ## Algebraic simplification rules that fire on identity_tensor
 *
 * - `dev(I) → 0`, `vol(I) → I`, `sym(I) → I`, `skew(I) → 0`
 *   (rank-2 forms in `tensor_functions.h`)
 * - `inv(I) → I` at any rank (self-inverse — true for the rank-2 Kronecker
 *   delta and for the rank-2R minor identity under its appropriate
 *   contraction).
 * - `trace(I) → dim`, `det(I) → 1` (rank-2 forms in
 *   `tensor_to_scalar_functions.cpp`).
 * - `tensor_mul`: `X · I = X` for rank-2 (the multiplicative identity for
 *   the rank-2 contraction; higher-rank cases fall through to the default).
 *
 * ## Construction
 *
 * - From user code: rarely needed directly — typically arises from
 *   `pow(A, 0)` or as a differentiation result.
 * - Explicit: `make_expression<identity_tensor>(dim, rank)`. `rank` must be
 *   even and at least 2.
 *
 * @see `tensor_data_identity` for the runtime evaluator.
 * @see #188 for the rationale behind unifying with the former
 * `kronecker_delta` node.
 */
class identity_tensor final : public tensor_node_base_t<identity_tensor> {
public:
  using base = tensor_node_base_t<identity_tensor>;

  identity_tensor() = delete;
  identity_tensor(std::size_t dim, std::size_t rank) : base(dim, rank) {
    // Pre-annotate the structural classification (SymPy-style closed-form
    // constant — see docs/sympy-assumption-redesign.md). Same pattern as
    // tensor_to_scalar_zero / tensor_to_scalar_one. Rank-2 identity is
    // the Kronecker δ_ij, which is symmetric. Rank-4 minor identity is
    // δ_ik·δ_jl, which has full minor + major symmetry. Higher ranks are
    // left unset for now — the variant has no general "all-pairs-minor"
    // alternative and higher-rank identity is rarely queried.
    if (rank == 2)
      this->set_space({Symmetric{}, AnyTraceTag{}});
    else if (rank == 4)
      this->set_space({MinorMajor{}, AnyTraceTag{}});
  }
  identity_tensor(identity_tensor &&data) noexcept
      : base(static_cast<base &&>(data), data.dim(), data.rank()) {
    // The 3-arg base ctor intentionally does NOT copy m_tensor_space (it's
    // used by n_ary_tree which manages space separately). Re-apply the
    // structural pre-annotation so copy/move preserve the closed-form
    // classification. Same footgun pattern as the step-1 tensor move-ctor
    // fix; locked in by IdentityTensorAssumptions.MovePreservesAnnotation.
    if (this->rank() == 2)
      this->set_space({Symmetric{}, AnyTraceTag{}});
    else if (this->rank() == 4)
      this->set_space({MinorMajor{}, AnyTraceTag{}});
  }
  identity_tensor(identity_tensor const &data)
      : base(static_cast<base const &>(data), data.dim(), data.rank()) {
    if (this->rank() == 2)
      this->set_space({Symmetric{}, AnyTraceTag{}});
    else if (this->rank() == 4)
      this->set_space({MinorMajor{}, AnyTraceTag{}});
  }
  ~identity_tensor() override = default;
  const identity_tensor &operator=(identity_tensor &&) = delete;

  friend bool operator<(identity_tensor const &lhs, identity_tensor const &rhs);
  friend bool operator>(identity_tensor const &lhs, identity_tensor const &rhs);
  friend bool operator==(identity_tensor const &lhs,
                         identity_tensor const &rhs);
  friend bool operator!=(identity_tensor const &lhs,
                         identity_tensor const &rhs);

  void update_hash_value() const override {
    base::m_hash_value = 0;
    hash_combine(base::m_hash_value, base::get_id());
    hash_combine(base::m_hash_value, this->dim());
    hash_combine(base::m_hash_value, this->rank());
  }
};

inline bool operator<(identity_tensor const &lhs, identity_tensor const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

inline bool operator>(identity_tensor const &lhs, identity_tensor const &rhs) {
  return rhs < lhs;
}

inline bool operator==(identity_tensor const &lhs, identity_tensor const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

inline bool operator!=(identity_tensor const &lhs, identity_tensor const &rhs) {
  return !(lhs == rhs);
}
} // namespace numsim::cas

#endif // IDENTITY_TENSOR_H
