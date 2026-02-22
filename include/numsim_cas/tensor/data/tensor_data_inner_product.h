#ifndef TENSOR_DATA_INNER_PRODUCT_H
#define TENSOR_DATA_INNER_PRODUCT_H

#include "tensor_data.h"
#include "tensor_data_basis_change.h"
#include <cstdlib>
#include <numsim_cas/core/cas_error.h>
#include <vector>

namespace numsim::cas {

template <typename ValueType>
class tensor_data_inner_product final
    : public tensor_data_eval_up_binary<tensor_data_inner_product<ValueType>,
                                        ValueType> {
public:
  tensor_data_inner_product(tensor_data_base<ValueType> &result,
                            tensor_data_base<ValueType> const &lhs,
                            tensor_data_base<ValueType> const &rhs,
                            std::vector<std::size_t> const &lhs_indices,
                            std::vector<std::size_t> const &rhs_indices)
      : m_result(result), m_lhs(lhs), m_rhs(rhs), m_lhs_indices(lhs_indices),
        m_rhs_indices(rhs_indices) {}

  template <std::size_t Dim, std::size_t RankLHS, std::size_t RankRHS>
  void evaluate_imp() {
    const auto size_lhs{m_lhs_indices.size()};
    const auto size_rhs{m_rhs_indices.size()};
    std::vector<std::size_t> lhs_indices(m_lhs_indices);
    std::vector<std::size_t> rhs_indices(m_rhs_indices);
    std::vector<std::size_t> lhs_sequence(RankLHS);
    std::vector<std::size_t> rhs_sequence(RankRHS);
    std::vector<std::size_t> sequence_outer_lhs;
    std::vector<std::size_t> sequence_outer_rhs;
    std::vector<std::size_t> new_basis_lhs;
    std::vector<std::size_t> new_basis_rhs;

    std::sort(lhs_indices.begin(), lhs_indices.end());
    std::sort(rhs_indices.begin(), rhs_indices.end());
    std::iota(lhs_sequence.begin(), lhs_sequence.end(), 0ul);
    std::iota(rhs_sequence.begin(), rhs_sequence.end(), 0ul);
    sequence_outer_lhs.reserve(RankLHS);
    std::set_difference(lhs_sequence.begin(), lhs_sequence.end(),
                        lhs_indices.begin(), lhs_indices.end(),
                        std::back_inserter(sequence_outer_lhs));
    sequence_outer_rhs.reserve(RankRHS);
    std::set_difference(rhs_sequence.begin(), rhs_sequence.end(),
                        rhs_indices.begin(), rhs_indices.end(),
                        std::back_inserter(sequence_outer_rhs));

    new_basis_lhs.reserve(RankLHS);
    new_basis_lhs.insert(new_basis_lhs.end(), sequence_outer_lhs.begin(),
                         sequence_outer_lhs.end());
    new_basis_lhs.insert(new_basis_lhs.end(), m_lhs_indices.begin(),
                         m_lhs_indices.end());

    new_basis_rhs.reserve(RankRHS);
    new_basis_rhs.insert(new_basis_rhs.end(), m_rhs_indices.begin(),
                         m_rhs_indices.end());
    new_basis_rhs.insert(new_basis_rhs.end(), sequence_outer_rhs.begin(),
                         sequence_outer_rhs.end());

    bool basis_change_lhs{false};
    if (lhs_sequence != new_basis_lhs) {
      basis_change_lhs = true;
      m_lhs_temp = make_tensor_data<ValueType>(Dim, RankLHS);
      tensor_data_basis_change<ValueType> basis_change_temp(
          *m_lhs_temp.get(), m_lhs, new_basis_lhs);
      basis_change_temp.evaluate(Dim, RankLHS);
    }

    bool basis_change_rhs{false};
    if (rhs_sequence != new_basis_rhs) {
      basis_change_rhs = true;
      m_rhs_temp = make_tensor_data<ValueType>(Dim, RankRHS);
      tensor_data_basis_change<ValueType> basis_change_temp(
          *m_rhs_temp.get(), m_rhs, new_basis_rhs);
      basis_change_temp.evaluate(Dim, RankRHS);
    }

    const auto RowsLHS{get_size(Dim, sequence_outer_lhs.size())};
    const auto ColsLHS{get_size(Dim, size_lhs)};
    const auto RowsRHS{get_size(Dim, size_rhs)};
    const auto ColsRHS{get_size(Dim, sequence_outer_rhs.size())};

    if (basis_change_lhs && basis_change_rhs) {
      evaluate_implementation(m_result.raw_data(), m_lhs_temp->raw_data(),
                              m_rhs_temp->raw_data(), RowsLHS, ColsLHS, RowsRHS,
                              ColsRHS);
    } else if (!basis_change_lhs && basis_change_rhs) {
      evaluate_implementation(m_result.raw_data(), m_lhs.raw_data(),
                              m_rhs_temp->raw_data(), RowsLHS, ColsLHS, RowsRHS,
                              ColsRHS);
    } else if (basis_change_lhs && !basis_change_rhs) {
      evaluate_implementation(m_result.raw_data(), m_lhs_temp->raw_data(),
                              m_rhs.raw_data(), RowsLHS, ColsLHS, RowsRHS,
                              ColsRHS);
    } else if (!basis_change_lhs && !basis_change_rhs) {
      evaluate_implementation(m_result.raw_data(), m_lhs.raw_data(),
                              m_rhs.raw_data(), RowsLHS, ColsLHS, RowsRHS,
                              ColsRHS);
    }
  }

  void mismatch(std::size_t dim, std::size_t rankLHS, std::size_t rankRHS) {
    if (dim > this->MaxDim_ || dim == 0) {
      throw evaluation_error("tensor_data_inner_product::evaluate(dim, "
                             "rankLHS, rankRHS) dim > MaxDim || dim == 0");
    }
    if (rankLHS > this->MaxRank_ || rankLHS == 0) {
      throw evaluation_error(
          "tensor_data_inner_product::evaluate(dim, rankLHS, rankRHS) rankLHS "
          "> MaxRank || rankLHS == 0");
    }
    if (rankRHS > this->MaxRank_ || rankRHS == 0) {
      throw evaluation_error(
          "tensor_data_inner_product::evaluate(dim, rankLHS, rankRHS) rankRHS "
          "> MaxRank || rankRHS == 0");
    }
  }

private:
  constexpr static void
  evaluate_implementation(ValueType *result, const ValueType *lhs,
                          const ValueType *rhs, std::size_t rows_lhs,
                          std::size_t columns_lhs, std::size_t rows_rhs,
                          std::size_t columns_rhs) noexcept {
    for (std::size_t i{0}; i < rows_lhs; ++i) {
      for (std::size_t j{0}; j < columns_rhs; ++j) {
        ValueType sum{0};
        for (std::size_t k{0}; k < rows_rhs; ++k) {
          sum += lhs[i * columns_lhs + k] * rhs[k * columns_rhs + j];
        }
        result[i * columns_rhs + j] = sum;
      }
    }
  }

  [[nodiscard]] constexpr static std::size_t
  get_size(std::size_t dim, std::size_t rank) noexcept {
    std::size_t size{1};
    for (std::size_t i{0}; i < rank; ++i) {
      size *= dim;
    }
    return size;
  }

  tensor_data_base<ValueType> &m_result;
  tensor_data_base<ValueType> const &m_lhs;
  tensor_data_base<ValueType> const &m_rhs;
  std::vector<std::size_t> const &m_lhs_indices;
  std::vector<std::size_t> const &m_rhs_indices;
  std::unique_ptr<tensor_data_base<ValueType>> m_lhs_temp;
  std::unique_ptr<tensor_data_base<ValueType>> m_rhs_temp;
};

} // namespace numsim::cas

#endif // TENSOR_DATA_INNER_PRODUCT_H
