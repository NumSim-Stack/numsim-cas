#ifndef TENSOR_DATA_SUB_H
#define TENSOR_DATA_SUB_H

#include "tensor_data.h"
#include <numsim_cas/core/cas_error.h>

namespace numsim::cas {

template <typename ValueType>
class tensor_data_sub final
    : public tensor_data_eval_up_unary<tensor_data_sub<ValueType>, ValueType> {
public:
  tensor_data_sub(tensor_data_base<ValueType> &lhs,
                  tensor_data_base<ValueType> const &rhs)
      : m_lhs(lhs), m_rhs(rhs) {}

  template <std::size_t Dim, std::size_t Rank>
  inline void evaluate_imp() noexcept {
    // using Tensor = tensor_data<T, Dim, Rank>;
    using Tensor = tensor_data<ValueType, Dim, Rank>;
    static_cast<Tensor &>(m_lhs).data() -=
        static_cast<const Tensor &>(m_rhs).data();
  }

  inline void mismatch(std::size_t dim, std::size_t rank) {
    if (dim > this->_MaxDim || dim == 0) {
      throw evaluation_error(
          "tensor_data_sub::evaluate(dim, rank) dim > MaxDim || dim == 0");
    }
    if (rank > this->_MaxRank || rank == 0) {
      throw evaluation_error("tensor_data_sub::evaluate(dim, rank) rank "
                             "> MaxRank || rank == 0");
    }
  }

private:
  tensor_data_base<ValueType> &m_lhs;
  tensor_data_base<ValueType> const &m_rhs;
};

} // namespace numsim::cas

#endif // TENSOR_DATA_SUB_H
