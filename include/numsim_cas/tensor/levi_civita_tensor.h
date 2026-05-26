#ifndef LEVI_CIVITA_TENSOR_H
#define LEVI_CIVITA_TENSOR_H

#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

/**
 * @class levi_civita_tensor
 * @brief The symbolic Levi-Civita symbol ε_{i₁…i_N} in N spatial dimensions.
 *
 * The Levi-Civita symbol (a.k.a. permutation symbol, totally-antisymmetric
 * symbol) at dimension N is a rank-N tensor whose components are:
 *
 *     ε_{i₁…i_N} = +1   if (i₁,…,i_N) is an even permutation of (1,…,N)
 *                  −1   if (i₁,…,i_N) is an odd permutation
 *                   0   if any two indices repeat
 *
 * It is the prototype antisymmetric tensor and the kernel of the standard
 * vector-calculus identities:
 *
 *   - **Cross product** (N=3): `(a × b)_i = ε_{ijk} a_j b_k`.
 *   - **Determinant** (N=3): `det(A) = ε_{ijk} A_{1i} A_{2j} A_{3k}`.
 *   - **Curl** (N=3): `(∇ × F)_i = ε_{ijk} ∂_j F_k`.
 *   - **Skew-to-vector axial map** (N=3): `ω_i = ½ ε_{ijk} W_{jk}` recovers
 *     the axial vector of a skew rank-2 tensor `W`.
 *
 * ## Rank equals dimension
 *
 * Unlike `identity_tensor` (which takes both `dim` and `rank` and represents
 * the rank-2R minor identity at any even rank), the Levi-Civita symbol is
 * intrinsically tied to its dimension: ε in N dimensions is *always* a
 * rank-N tensor. There is no rank-3 ε in 2D, and no rank-2 ε in 3D. The
 * constructor therefore takes only `dim`.
 *
 * ## Supported dimensions
 *
 * Dimensions 2, 3, and 4 are supported (matching `tmech::levi_civita`'s
 * coverage). The rank-2 case (ε_{ij} in 2D) is the 2×2 skew unit
 * `[[0, 1], [−1, 0]]`. The runtime evaluator
 * `tensor_data_levi_civita` rejects other dimensions at evaluation time.
 *
 * ## Differentiation
 *
 * ε has no symbolic dependence on any user variable, so
 * `diff(ε, X) = 0` for every X. The differentiation visitor leaves the
 * accumulator at its default-constructed value (the zero tensor), mirroring
 * what `identity_tensor` does.
 *
 * ## Construction
 *
 * - From user code: `levi_civita(dim)` (free function in `tensor_std.h`)
 *   or `levi_civita<3>()` if the dimension is a compile-time constant.
 * - Explicit: `make_expression<levi_civita_tensor>(dim)`.
 *
 * @see `tensor_data_levi_civita` for the runtime evaluator.
 * @see #34 for the original feature request.
 */
class levi_civita_tensor final : public tensor_node_base_t<levi_civita_tensor> {
public:
  using base = tensor_node_base_t<levi_civita_tensor>;

  levi_civita_tensor() = delete;

  // Rank is fixed by dimension — base takes (dim, rank).
  explicit levi_civita_tensor(std::size_t dim) : base(dim, dim) {}

  levi_civita_tensor(levi_civita_tensor &&data) noexcept
      : base(static_cast<base &&>(data), data.dim(), data.rank()) {}
  levi_civita_tensor(levi_civita_tensor const &data)
      : base(static_cast<base const &>(data), data.dim(), data.rank()) {}
  ~levi_civita_tensor() override = default;
  const levi_civita_tensor &operator=(levi_civita_tensor &&) = delete;

  friend bool operator<(levi_civita_tensor const &lhs,
                        levi_civita_tensor const &rhs);
  friend bool operator>(levi_civita_tensor const &lhs,
                        levi_civita_tensor const &rhs);
  friend bool operator==(levi_civita_tensor const &lhs,
                         levi_civita_tensor const &rhs);
  friend bool operator!=(levi_civita_tensor const &lhs,
                         levi_civita_tensor const &rhs);

  void update_hash_value() const override {
    base::m_hash_value = 0;
    hash_combine(base::m_hash_value, base::get_id());
    hash_combine(base::m_hash_value, this->dim());
    // rank == dim, no need to fold it in separately.
  }
};

inline bool operator<(levi_civita_tensor const &lhs,
                      levi_civita_tensor const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

inline bool operator>(levi_civita_tensor const &lhs,
                      levi_civita_tensor const &rhs) {
  return rhs < lhs;
}

inline bool operator==(levi_civita_tensor const &lhs,
                       levi_civita_tensor const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

inline bool operator!=(levi_civita_tensor const &lhs,
                       levi_civita_tensor const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // LEVI_CIVITA_TENSOR_H
