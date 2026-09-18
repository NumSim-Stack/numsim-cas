#ifndef TENSOR_SHAPE_VALIDATION_H
#define TENSOR_SHAPE_VALIDATION_H

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/tensor/sequence.h>
#include <numsim_cas/tensor/tensor_expression.h>

#include <string>

// Construction-time shape checks for tensor operations. They throw
// invalid_expression_error (never assert) so malformed nodes cannot reach
// the evaluator in Release builds.

namespace numsim::cas::detail {

inline std::string shape_str(tensor_expression const &e) {
  return "dim " + std::to_string(e.dim()) + ", rank " +
         std::to_string(e.rank());
}

inline void validate_same_shape(char const *op, tensor_expression const &lhs,
                                tensor_expression const &rhs) {
  if (lhs.dim() != rhs.dim() || lhs.rank() != rhs.rank())
    throw invalid_expression_error(
        std::string(op) + ": operands must share dim and rank (" +
        shape_str(lhs) + " vs " + shape_str(rhs) + ")");
}

inline void validate_same_dim(char const *op, tensor_expression const &lhs,
                              tensor_expression const &rhs) {
  if (lhs.dim() != rhs.dim())
    throw invalid_expression_error(
        std::string(op) + ": operands must share dim (" +
        std::to_string(lhs.dim()) + " vs " + std::to_string(rhs.dim()) + ")");
}

// A permutation must cover exactly the operand's index positions: a mismatch
// would drop or invent indices, and a node whose sequence disagrees with its
// rank misreports itself to callers such as is_trans_of.
inline void validate_permutation(char const *op, tensor_expression const &e,
                                 sequence const &indices) {
  const auto rank = e.rank();
  if (indices.size() != rank)
    throw invalid_expression_error(
        std::string(op) + ": indices size (" + std::to_string(indices.size()) +
        ") must equal tensor rank (" + std::to_string(rank) + ")");
  for (std::size_t i = 0; i < indices.size(); ++i) {
    if (indices[i] >= rank)
      throw invalid_expression_error(std::string(op) + ": index " +
                                     std::to_string(indices[i] + 1) +
                                     " (1-based) is out of range for rank-" +
                                     std::to_string(rank) + " tensor");
    for (std::size_t j = i + 1; j < indices.size(); ++j)
      if (indices[i] == indices[j])
        throw invalid_expression_error(
            std::string(op) + ": index " + std::to_string(indices[i] + 1) +
            " (1-based) appears more than once; indices must form a "
            "permutation");
  }
}

// Every index is a valid position below `bound` and appears once.
inline void validate_distinct_indices(char const *op, char const *side,
                                      sequence const &indices,
                                      std::size_t bound) {
  for (std::size_t i = 0; i < indices.size(); ++i) {
    if (indices[i] >= bound)
      throw invalid_expression_error(std::string(op) + ": " + side + " index " +
                                     std::to_string(indices[i] + 1) +
                                     " (1-based) is out of range (" +
                                     std::to_string(bound) + " positions)");
    for (std::size_t j = i + 1; j < indices.size(); ++j)
      if (indices[i] == indices[j])
        throw invalid_expression_error(std::string(op) + ": " + side +
                                       " index " +
                                       std::to_string(indices[i] + 1) +
                                       " (1-based) appears more than once");
  }
}

// Contraction of lhs_indices with rhs_indices, pairwise.
inline void validate_contraction(char const *op, tensor_expression const &lhs,
                                 sequence const &lhs_indices,
                                 tensor_expression const &rhs,
                                 sequence const &rhs_indices) {
  validate_same_dim(op, lhs, rhs);
  if (lhs_indices.size() != rhs_indices.size())
    throw invalid_expression_error(std::string(op) +
                                   ": contraction index counts differ (" +
                                   std::to_string(lhs_indices.size()) + " vs " +
                                   std::to_string(rhs_indices.size()) + ")");
  validate_distinct_indices(op, "lhs", lhs_indices, lhs.rank());
  validate_distinct_indices(op, "rhs", rhs_indices, rhs.rank());
}

// Contraction over every index of both operands (scalar result).
inline void validate_full_contraction(char const *op,
                                      tensor_expression const &lhs,
                                      sequence const &lhs_indices,
                                      tensor_expression const &rhs,
                                      sequence const &rhs_indices) {
  if (lhs_indices.size() != lhs.rank() || rhs_indices.size() != rhs.rank())
    throw invalid_expression_error(
        std::string(op) + ": a scalar contraction must use every index (" +
        std::to_string(lhs_indices.size()) + " of rank " +
        std::to_string(lhs.rank()) + ", " + std::to_string(rhs_indices.size()) +
        " of rank " + std::to_string(rhs.rank()) + ")");
  validate_contraction(op, lhs, lhs_indices, rhs, rhs_indices);
}

// lhs_indices / rhs_indices name the result positions of each operand's
// indices, so together they must be a permutation of the result's positions.
inline void validate_outer_product(char const *op, tensor_expression const &lhs,
                                   sequence const &lhs_indices,
                                   tensor_expression const &rhs,
                                   sequence const &rhs_indices) {
  validate_same_dim(op, lhs, rhs);
  if (lhs_indices.size() != lhs.rank() || rhs_indices.size() != rhs.rank())
    throw invalid_expression_error(
        std::string(op) + ": index counts must equal operand ranks (" +
        std::to_string(lhs_indices.size()) + " vs rank " +
        std::to_string(lhs.rank()) + ", " + std::to_string(rhs_indices.size()) +
        " vs rank " + std::to_string(rhs.rank()) + ")");
  sequence all(lhs_indices.size() + rhs_indices.size());
  std::size_t k = 0;
  for (auto i : lhs_indices)
    all[k++] = i;
  for (auto i : rhs_indices)
    all[k++] = i;
  validate_distinct_indices(op, "result", all, all.size());
}

} // namespace numsim::cas::detail

#endif // TENSOR_SHAPE_VALIDATION_H
