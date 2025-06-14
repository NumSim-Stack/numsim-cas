#ifndef MAKE_TENSOR_DATA_IMP_H
#define MAKE_TENSOR_DATA_IMP_H

#include "../../numsim_cas_type_traits.h"
#include "tensor_data.h"

namespace numsim::cas {

template <typename ValueType>
class make_tensor_data_imp final
    : public tensor_data_eval_up_unary<make_tensor_data_imp<ValueType>,
                                       ValueType> {
public:
  make_tensor_data_imp() = default;
  make_tensor_data_imp(make_tensor_data_imp const &) = delete;
  make_tensor_data_imp(make_tensor_data_imp &&) = delete;
  ~make_tensor_data_imp() = default;
  const make_tensor_data_imp &operator=(make_tensor_data_imp const &) = delete;

  template <std::size_t Dim, std::size_t Rank>
  [[nodiscard]] std::unique_ptr<tensor_data_base<ValueType>>
  evaluate_imp() noexcept {
    return std::make_unique<tensor_data<ValueType, Dim, Rank>>();
  }

  [[nodiscard]] std::unique_ptr<tensor_data_base<ValueType>>
  missmatch(std::size_t dim, std::size_t rank) {
    if (dim > this->_MaxDim || dim == 0) {
      throw std::runtime_error(
          "make_tensor_data_imp::evaluate(dim, rank) dim > MaxDim || dim == 0");
    }
    if (rank > this->_MaxRank || rank == 0) {
      throw std::runtime_error("make_tensor_data_imp::evaluate(dim, rank) rank "
                               "> MaxRank || rank == 0");
    }
    return nullptr;
  }
};

} // namespace numsim::cas
#endif // MAKE_TENSOR_DATA_IMP_H
