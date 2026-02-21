#ifndef SCALAR_TENSOR_OP_H
#define SCALAR_TENSOR_OP_H

#include "data/tensor_data.h"
#include <numsim_cas/core/cas_error.h>

namespace numsim::cas {

namespace detail {
struct tensor_scalar_mul {
  template <typename Scalar, typename Tensor>
  static constexpr inline auto apply(Scalar const &scalar,
                                     Tensor const &tensor) {
    return scalar * tensor;
  }
};

struct tensor_scalar_div {
  template <typename Scalar, typename Tensor>
  static constexpr inline auto apply(Scalar const &scalar,
                                     Tensor const &tensor) {
    return tensor / scalar;
  }
};
} // namespace detail

template <typename OP, typename ValueType>
class scalar_tensor_op final
    : public tensor_data_eval_up_unary<scalar_tensor_op<OP, ValueType>,
                                       ValueType> {
public:
  scalar_tensor_op(tensor_data_base<ValueType> &lhs, ValueType const &scalar,
                   tensor_data_base<ValueType> const &rhs)
      : m_lhs(lhs), m_scalar(scalar), m_rhs(rhs) {}

  template <typename T, std::size_t Dim, std::size_t Rank>
  inline void evaluate_imp() noexcept {
    // using Tensor = tensor_data<T, Dim, Rank>;
    using Tensor = tensor_data<T, Dim, Rank>;
    static_cast<Tensor &>(m_lhs).data() +=
        OP::apply(m_scalar, static_cast<const Tensor &>(m_rhs).data());
  }

  inline void mismatch(std::size_t dim, std::size_t rank) {
    if (dim > this->_MaxDim || dim == 0) {
      throw evaluation_error(
          "tensor_data_add::evaluate(dim, rank) dim > MaxDim || dim == 0");
    }
    if (rank > this->_MaxRank || rank == 0) {
      throw evaluation_error("tensor_data_add::evaluate(dim, rank) rank "
                             "> MaxRank || rank == 0");
    }
  }

private:
  tensor_data_base<ValueType> &m_lhs;
  ValueType const &m_scalar;
  tensor_data_base<ValueType> const &m_rhs;
};

using scalar_tensor_mul_op =
    scalar_tensor_op<detail::tensor_scalar_mul, double>;
using scalar_tensor_div_op =
    scalar_tensor_op<detail::tensor_scalar_div, double>;

} // namespace numsim::cas

#endif // SCALAR_TENSOR_OP_H
