#ifndef TENSOR_DATA_SCALAR_MUL_H
#define TENSOR_DATA_SCALAR_MUL_H

#include "tensor_data.h"
#include <numsim_cas/core/cas_error.h>

namespace numsim::cas {

template <typename ValueType>
class tensor_data_scalar_mul final
    : public tensor_data_eval_up_unary<tensor_data_scalar_mul<ValueType>,
                                       ValueType> {
public:
  tensor_data_scalar_mul(tensor_data_base<ValueType> &dst,
                         tensor_data_base<ValueType> const &src,
                         ValueType scalar)
      : m_dst(dst), m_src(src), m_scalar(scalar) {}

  template <std::size_t Dim, std::size_t Rank>
  inline void evaluate_imp() noexcept {
    using ::operator*
        ; // tmech defines operator*(scalar, tensor) at global scope
    using Tensor = tensor_data<ValueType, Dim, Rank>;
    static_cast<Tensor &>(m_dst).data() =
        m_scalar * static_cast<const Tensor &>(m_src).data();
  }

  inline void mismatch(std::size_t dim, std::size_t rank) {
    if (dim > this->MaxDim_ || dim == 0) {
      throw evaluation_error("tensor_data_scalar_mul::evaluate(dim, rank) dim "
                             "> MaxDim || dim == 0");
    }
    if (rank > this->MaxRank_ || rank == 0) {
      throw evaluation_error("tensor_data_scalar_mul::evaluate(dim, rank) rank "
                             "> MaxRank || rank == 0");
    }
  }

private:
  tensor_data_base<ValueType> &m_dst;
  tensor_data_base<ValueType> const &m_src;
  ValueType m_scalar;
};

} // namespace numsim::cas

#endif // TENSOR_DATA_SCALAR_MUL_H
