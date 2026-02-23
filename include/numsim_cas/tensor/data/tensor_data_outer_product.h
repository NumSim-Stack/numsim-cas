#ifndef TENSOR_DATA_OUTER_PRODUCT_H
#define TENSOR_DATA_OUTER_PRODUCT_H

#include "../../numsim_cas_type_traits.h"
#include "tensor_data.h"
#include <numsim_cas/core/cas_error.h>

namespace numsim::cas {

template <typename ValueType>
class tensor_data_outer_product final
    : public tensor_data_eval_up_binary<tensor_data_outer_product<ValueType>,
                                        ValueType> {
public:
  tensor_data_outer_product(tensor_data_base<ValueType> &result,
                            tensor_data_base<ValueType> const &lhs,
                            tensor_data_base<ValueType> const &rhs,
                            std::vector<std::size_t> const &lhs_indices,
                            std::vector<std::size_t> const &rhs_indices)
      : m_result(result), m_lhs(lhs), m_rhs(rhs), m_lhs_indices(lhs_indices),
        m_rhs_indices(rhs_indices) {}

  template <std::size_t Dim, std::size_t RankLHS, std::size_t RankRHS>
  void evaluate_imp() {
    using TensorResult = tensor_data<ValueType, Dim, RankLHS + RankRHS>;
    using TensorLHS = tensor_data<ValueType, Dim, RankLHS>;
    using TensorRHS = tensor_data<ValueType, Dim, RankRHS>;

    auto func = [&](auto... args) {
      using sequence_lhs = std::make_index_sequence<RankLHS>;
      using sequence_rhs = std::make_index_sequence<RankRHS>;

      const std::array<std::size_t, RankLHS + RankRHS> input{args...};
      auto &result{static_cast<TensorResult &>(m_result).data()};
      const auto &lhs{static_cast<const TensorLHS &>(m_lhs).data()};
      const auto &rhs{static_cast<const TensorRHS &>(m_rhs).data()};
      result(args...) = tuple_call(lhs, m_lhs_indices, input, sequence_lhs()) *
                        tuple_call(rhs, m_rhs_indices, input, sequence_rhs());
    };
    tmech::detail::for_loop_t<RankLHS + RankRHS - 1, Dim>::for_loop(func);
  }

  void mismatch(std::size_t dim, std::size_t rankLHS, std::size_t rankRHS) {
    if (dim > this->MaxDim_ || dim == 0) {
      throw evaluation_error("tensor_data_outer_product::evaluate(dim, "
                             "rankLHS, rankRHS) dim > MaxDim || dim == 0");
    }
    if (rankLHS > this->MaxRank_ || rankLHS == 0) {
      throw evaluation_error(
          "tensor_data_outer_product::evaluate(dim, rankLHS, rankRHS) rankLHS "
          "> MaxRank || rankLHS == 0");
    }
    if (rankRHS > this->MaxRank_ || rankRHS == 0) {
      throw evaluation_error(
          "tensor_data_outer_product::evaluate(dim, rankLHS, rankRHS) rankRHS "
          "> MaxRank || rankRHS == 0");
    }
  }

private:
  template <typename F, typename Vec, typename Arg, std::size_t... INDICES>
  static constexpr inline auto
  tuple_call(F const &f, Vec const &vec, Arg const &arg,
             [[maybe_unused]] std::index_sequence<INDICES...> seq) noexcept {
    return f(arg[vec[INDICES]]...);
  }

  tensor_data_base<ValueType> &m_result;
  tensor_data_base<ValueType> const &m_lhs;
  tensor_data_base<ValueType> const &m_rhs;
  std::vector<std::size_t> const &m_lhs_indices;
  std::vector<std::size_t> const &m_rhs_indices;
};

} // namespace numsim::cas

#endif // TENSOR_DATA_OUTER_PRODUCT_H
