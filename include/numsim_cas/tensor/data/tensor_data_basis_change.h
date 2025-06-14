#ifndef TENSOR_DATA_BASIS_CHANGE_H
#define TENSOR_DATA_BASIS_CHANGE_H

#include "tensor_data.h"

namespace numsim::cas {

template <typename ValueType>
class tensor_data_basis_change final
    : public tensor_data_eval_up_unary<tensor_data_basis_change<ValueType>,
                                       ValueType> {
public:
  tensor_data_basis_change(tensor_data_base<ValueType> &lhs,
                           tensor_data_base<ValueType> const &rhs,
                           std::vector<std::size_t> const &indices)
      : m_lhs(lhs), m_rhs(rhs), m_indices(indices) {}

  tensor_data_basis_change(tensor_data_basis_change const &) = delete;
  tensor_data_basis_change(tensor_data_basis_change &&) = delete;
  ~tensor_data_basis_change() = default;
  const tensor_data_basis_change &
  operator=(tensor_data_basis_change const &) = delete;

  template <std::size_t Dim, std::size_t Rank>
  inline void evaluate_imp() noexcept {
    using Tensor = tensor_data<ValueType, Dim, Rank>;
    auto func = [&](auto... args) {
      using sequence = std::make_index_sequence<Rank>;
      std::array<std::size_t, Rank> input{args...};
      auto &lhs{static_cast<Tensor &>(m_lhs).data()};
      const auto &rhs{static_cast<const Tensor &>(m_rhs).data()};
      lhs(args...) = tuple_call(rhs, m_indices, input, sequence());
    };
    tmech::detail::for_loop_t<Rank - 1, Dim>::for_loop(func);
  }

  inline void missmatch(std::size_t dim, std::size_t rank) {
    if (dim > this->_MaxDim || dim == 0) {
      throw std::runtime_error("tensor_data_basis_change::evaluate(dim, rank) "
                               "dim > MaxDim || dim == 0");
    }
    if (rank > this->_MaxRank || rank == 0) {
      throw std::runtime_error(
          "tensor_data_basis_change::evaluate(dim, rank) rank "
          "> MaxRank || rank == 0");
    }
  }

private:
  template <typename F, typename Vec, typename Arg, std::size_t... INDICES>
  static constexpr inline auto
  tuple_call(F const &f, Vec const &vec, Arg const &arg,
             std::index_sequence<INDICES...>) noexcept {
    return f(arg[vec[INDICES]]...);
  }

  tensor_data_base<ValueType> &m_lhs;
  tensor_data_base<ValueType> const &m_rhs;
  std::vector<std::size_t> const &m_indices;
};

} // namespace numsim::cas

#endif // TENSOR_DATA_BASIS_CHANGE_H
