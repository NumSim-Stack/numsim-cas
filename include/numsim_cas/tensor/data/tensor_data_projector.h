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
      // Rank-8 projectors (acting on rank-4 tensors) for #299. Used by
      // the diff visitor's leaf rule for rank-4 annotated variables.
      // Constructed component-wise as Reynolds projectors over the
      // input-pair group (Z_2 × Z_2 for Minor, D_4 for MinorMajor) —
      // see projection_tensor.h::P_minor4 / P_minor_major4 for the
      // group definitions.
      using Tensor = tensor_data<ValueType, Dim, Rank>;
      auto &data = static_cast<Tensor &>(m_result).data();

      if (std::holds_alternative<Minor>(m_space.perm) &&
          std::holds_alternative<AnyTraceTag>(m_space.trace)) {
        // P_minor_{ijkl,mnpq} = (1/4) Σ_σ δ_{i,σ(m)} δ_{j,σ(n)}
        //                              δ_{k,σ(p)} δ_{l,σ(q)}
        // where σ ∈ {id, swap(m,n), swap(p,q), swap both}.
        for (std::size_t i = 0; i < Dim; ++i)
          for (std::size_t j = 0; j < Dim; ++j)
            for (std::size_t k = 0; k < Dim; ++k)
              for (std::size_t l = 0; l < Dim; ++l)
                for (std::size_t m = 0; m < Dim; ++m)
                  for (std::size_t n = 0; n < Dim; ++n)
                    for (std::size_t p = 0; p < Dim; ++p)
                      for (std::size_t q = 0; q < Dim; ++q) {
                        ValueType acc{};
                        // id
                        acc += (i == m) * (j == n) * (k == p) * (l == q);
                        // swap (m,n)
                        acc += (i == n) * (j == m) * (k == p) * (l == q);
                        // swap (p,q)
                        acc += (i == m) * (j == n) * (k == q) * (l == p);
                        // swap both
                        acc += (i == n) * (j == m) * (k == q) * (l == p);
                        data(i, j, k, l, m, n, p, q) = ValueType{0.25} * acc;
                      }
      } else if (std::holds_alternative<MinorMajor>(m_space.perm) &&
                 std::holds_alternative<AnyTraceTag>(m_space.trace)) {
        // P_minor_major_{ijkl,mnpq} = (1/8) Σ_σ δ_{i,σ(m)} ... where σ
        // ranges over the D_4 group (Minor's 4 + major-swap composed
        // with each Minor element).
        for (std::size_t i = 0; i < Dim; ++i)
          for (std::size_t j = 0; j < Dim; ++j)
            for (std::size_t k = 0; k < Dim; ++k)
              for (std::size_t l = 0; l < Dim; ++l)
                for (std::size_t m = 0; m < Dim; ++m)
                  for (std::size_t n = 0; n < Dim; ++n)
                    for (std::size_t p = 0; p < Dim; ++p)
                      for (std::size_t q = 0; q < Dim; ++q) {
                        ValueType acc{};
                        // Minor's 4 elements
                        acc += (i == m) * (j == n) * (k == p) * (l == q);
                        acc += (i == n) * (j == m) * (k == p) * (l == q);
                        acc += (i == m) * (j == n) * (k == q) * (l == p);
                        acc += (i == n) * (j == m) * (k == q) * (l == p);
                        // Major-swap composed with Minor's 4
                        acc += (i == p) * (j == q) * (k == m) * (l == n);
                        acc += (i == q) * (j == p) * (k == m) * (l == n);
                        acc += (i == p) * (j == q) * (k == n) * (l == m);
                        acc += (i == q) * (j == p) * (k == n) * (l == m);
                        data(i, j, k, l, m, n, p, q) = ValueType{0.125} * acc;
                      }
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
