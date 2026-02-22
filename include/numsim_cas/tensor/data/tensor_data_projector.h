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
    } else {
      throw not_implemented_error(
          "tensor_data_projector: only rank-4 projectors supported");
    }
  }

  void mismatch(std::size_t dim, std::size_t rank) {
    if (dim > this->MaxDim_ || dim == 0)
      throw evaluation_error(
          "tensor_data_projector: dim > MaxDim || dim == 0");
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
