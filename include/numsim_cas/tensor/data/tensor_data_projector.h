#ifndef TENSOR_DATA_PROJECTOR_H
#define TENSOR_DATA_PROJECTOR_H

#include "tensor_data.h"
#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/tensor/tensor_space.h>

namespace numsim::cas {

template <typename ValueType>
class tensor_data_projector final
    : public tensor_data_eval_up_unary<tensor_data_projector<ValueType>,
                                       ValueType> {
public:
  tensor_data_projector(tensor_data_base<ValueType> &result,
                        tensor_space const &space)
      : m_result(result), m_space(space) {}

  template <std::size_t Dim, std::size_t Rank> void evaluate_imp() {
    // tmech defines operator+, operator-, operator* in the global namespace;
    // bring them into scope so they aren't hidden by numsim::cas::operator*.
    using ::operator+;
    using ::operator-;
    using ::operator*;

    if constexpr (Rank == 4) {
      using Tensor = tensor_data<ValueType, Dim, Rank>;
      auto &data = static_cast<Tensor &>(m_result).data();
      auto I2 = tmech::eye<ValueType, Dim, 2>();

      if (std::holds_alternative<Symmetric>(m_space.perm) &&
          std::holds_alternative<AnyTraceTag>(m_space.trace)) {
        // P_sym = 0.5 * (otimesu(I, I) + otimesl(I, I))
        data = 0.5 * (tmech::otimesu(I2, I2) + tmech::otimesl(I2, I2));
      } else if (std::holds_alternative<Skew>(m_space.perm) &&
                 std::holds_alternative<AnyTraceTag>(m_space.trace)) {
        // P_skew = 0.5 * (otimesu(I, I) - otimesl(I, I))
        data = 0.5 * (tmech::otimesu(I2, I2) - tmech::otimesl(I2, I2));
      } else if (std::holds_alternative<Symmetric>(m_space.perm) &&
                 std::holds_alternative<VolumetricTag>(m_space.trace)) {
        // P_vol = (1/Dim) * otimes(I, I)
        constexpr auto inv_d = ValueType{1} / static_cast<ValueType>(Dim);
        data = inv_d * tmech::otimes(I2, I2);
      } else if (std::holds_alternative<Symmetric>(m_space.perm) &&
                 std::holds_alternative<DeviatoricTag>(m_space.trace)) {
        // P_dev = P_sym - P_vol
        constexpr auto inv_d = ValueType{1} / static_cast<ValueType>(Dim);
        data = 0.5 * (tmech::otimesu(I2, I2) + tmech::otimesl(I2, I2)) -
               inv_d * tmech::otimes(I2, I2);
      } else {
        throw not_implemented_error(
            "tensor_data_projector: unsupported projector space");
      }
    } else if constexpr (Rank == 8) {
      // Rank-8 projectors (acting on rank-4 tensors) for #299 + Major
      // follow-up. Used by the diff visitor's leaf rule for rank-4
      // annotated variables.
      //
      // Constructed via tmech::otimes + basis_change rather than
      // raw 8-nested loops. The building block is a rank-4 tensor
      // (P_sym_r4 or I4_uu) that's outer-producted to rank-8 and
      // then permuted so the four δ-pairs land at the desired
      // positions. Conventions:
      //   - Minor:      4-term Z_2×Z_2 symmetrizer over (m,n) and
      //                 (p,q) input swaps, pair structure
      //                 (1,5)(2,6)(3,7)(4,8).
      //   - MinorMajor: P_minor plus its major-swap (input pair
      //                 (m,n)↔(p,q), positions (5,6)↔(7,8)). 8-term
      //                 D_4 symmetrizer.
      //   - Major:      2-element D_2 group with the major-pair swap
      //                 only — no minor symmetrization.
      //
      // tmech basis_change<seq<p_1,…,p_N>>(T)[k_1,…,k_N]
      // = T[k_{p_1},…,k_{p_N}] (1-based) — see tmech_utility.h's
      // tuple_call. The same kernel structure is exercised by
      // TensorDiffRank4Inv lock-ins and the FuzzyTensorDiffTest
      // symmetry-projecting FD path.
      using Tensor = tensor_data<ValueType, Dim, Rank>;
      auto &data = static_cast<Tensor &>(m_result).data();
      auto I2 = tmech::eye<ValueType, Dim, 2>();
      // P_sym_r4_{abcd} = (1/2)(δ_ac δ_bd + δ_ad δ_bc) — rank-4 minor-
      // symmetric identity; building block for Minor / MinorMajor.
      auto P_sym_r4 =
          ValueType{0.5} * (tmech::otimesu(I2, I2) + tmech::otimesl(I2, I2));
      // I4_uu_{abcd} = δ_ac δ_bd — rank-4 minor identity without
      // second-pair symmetrization; building block for Major.
      auto I4_uu = tmech::otimesu(I2, I2);

      if (std::holds_alternative<Minor>(m_space.perm) &&
          std::holds_alternative<AnyTraceTag>(m_space.trace)) {
        // P_minor = basis_change(otimes(P_sym_r4, P_sym_r4)) ∈ R^{rank-8}.
        // Natural pair structure (1,3)(2,4) and (5,7)(6,8); permute to
        // (1,5)(2,6)(3,7)(4,8). Expands to
        // (1/4)(δ_im δ_jn + δ_in δ_jm)(δ_kp δ_lq + δ_kq δ_lp).
        data = tmech::basis_change<tmech::sequence<1, 2, 5, 6, 3, 4, 7, 8>>(
            tmech::otimes(P_sym_r4, P_sym_r4));
      } else if (std::holds_alternative<MinorMajor>(m_space.perm) &&
                 std::holds_alternative<AnyTraceTag>(m_space.trace)) {
        // P_mm = (1/2)(P_minor + σ_major(P_minor)) where σ_major swaps
        // input positions (5,6) ↔ (7,8) — i.e. the (m,n) ↔ (p,q)
        // major-pair swap. The 4 Minor terms × 2 = 8-term D_4 sum.
        auto P_minor =
            tmech::basis_change<tmech::sequence<1, 2, 5, 6, 3, 4, 7, 8>>(
                tmech::otimes(P_sym_r4, P_sym_r4));
        data = ValueType{0.5} *
               (P_minor +
                tmech::basis_change<tmech::sequence<1, 2, 3, 4, 7, 8, 5, 6>>(
                    P_minor));
      } else if (std::holds_alternative<Major>(m_space.perm) &&
                 std::holds_alternative<AnyTraceTag>(m_space.trace)) {
        // P_major = (1/2)(T_id + T_major_swap) where each Tᵢ is a
        // basis_change of otimes(I4_uu, I4_uu) — no minor
        // symmetrization. Equals
        // (1/2)(δ_im δ_jn δ_kp δ_lq + δ_ip δ_jq δ_km δ_ln).
        auto natural = tmech::otimes(I4_uu, I4_uu);
        data = ValueType{0.5} *
               (tmech::basis_change<tmech::sequence<1, 2, 5, 6, 3, 4, 7, 8>>(
                    natural) +
                tmech::basis_change<tmech::sequence<1, 2, 7, 8, 3, 4, 5, 6>>(
                    natural));
      } else {
        throw not_implemented_error(
            "tensor_data_projector: unsupported rank-8 projector space");
      }
    } else {
      throw not_implemented_error(
          "tensor_data_projector: only rank-4 and rank-8 projectors supported");
    }
  }

  void mismatch(std::size_t dim, std::size_t rank) {
    if (dim > this->MaxDim_ || dim == 0)
      throw evaluation_error("tensor_data_projector: dim > MaxDim || dim == 0");
    if (rank > this->MaxRank_ || rank == 0)
      throw evaluation_error(
          "tensor_data_projector: rank > MaxRank || rank == 0");
  }

private:
  tensor_data_base<ValueType> &m_result;
  tensor_space const &m_space;
};

} // namespace numsim::cas

#endif // TENSOR_DATA_PROJECTOR_H
