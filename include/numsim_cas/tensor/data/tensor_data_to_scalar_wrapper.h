#ifndef TENSOR_DATA_TO_SCALAR_WRAPPER_H
#define TENSOR_DATA_TO_SCALAR_WRAPPER_H

#include "spectral_decomposition_cache.h"
#include "tensor_data.h"
#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/tensor/sequence.h>

#include <algorithm>
#include <array>

namespace numsim::cas {

// ─── Generic unary wrapper: dispatches runtime (dim,rank) to tmech Op
//     that returns a scalar ValueType (not a tensor) ─────────────────────
//
// Op must provide:
//   static constexpr bool is_valid<Dim, Rank>()
//   static auto apply(tmech_tensor const&) → scalar
//
template <typename Op, typename ValueType>
class tensor_data_to_scalar_wrapper final
    : public tensor_data_eval_up_unary<
          tensor_data_to_scalar_wrapper<Op, ValueType>, ValueType> {
public:
  explicit tensor_data_to_scalar_wrapper(
      tensor_data_base<ValueType> const &input)
      : m_input(input) {}

  template <std::size_t Dim, std::size_t Rank> ValueType evaluate_imp() {
    if constexpr (Op::template is_valid<Dim, Rank>()) {
      using Tensor = tensor_data<ValueType, Dim, Rank>;
      auto const &in = static_cast<const Tensor &>(m_input).data();
      return static_cast<ValueType>(Op::apply(in));
    } else {
      throw evaluation_error("tensor_data_to_scalar_wrapper: invalid dim/rank");
    }
  }

  ValueType mismatch(std::size_t dim, std::size_t rank) {
    if (dim > this->MaxDim_ || dim == 0)
      throw evaluation_error(
          "tensor_data_to_scalar_wrapper: dim > MaxDim || dim == 0");
    if (rank > this->MaxRank_ || rank == 0)
      throw evaluation_error(
          "tensor_data_to_scalar_wrapper: rank > MaxRank || rank == 0");
    return ValueType{};
  }

private:
  tensor_data_base<ValueType> const &m_input;
};

// ─── Binary dcontract wrapper: dispatches runtime (dim,rank) and computes
//     tmech::dcontract(lhs, rhs) → scalar ────────────────────────────────
//
template <typename ValueType>
class tensor_data_dcontract_wrapper final
    : public tensor_data_eval_up_unary<tensor_data_dcontract_wrapper<ValueType>,
                                       ValueType> {
public:
  // #353 — the contraction sequences are part of the node semantics:
  // {1,2}/{2,1} is A_ij*B_ji (A : B^T), not A : B.
  tensor_data_dcontract_wrapper(tensor_data_base<ValueType> const &lhs,
                                tensor_data_base<ValueType> const &rhs,
                                sequence const &lhs_indices,
                                sequence const &rhs_indices)
      : m_lhs(lhs), m_rhs(rhs), m_lhs_indices(lhs_indices),
        m_rhs_indices(rhs_indices) {}

  template <std::size_t Dim, std::size_t Rank> ValueType evaluate_imp() {
    if constexpr (Rank == 2) {
      using Tensor = tensor_data<ValueType, Dim, Rank>;
      auto const &l = static_cast<const Tensor &>(m_lhs).data();
      auto const &r = static_cast<const Tensor &>(m_rhs).data();
      const bool straight{m_lhs_indices == m_rhs_indices};
      if (straight) {
        // {1,2}/{1,2} = A_ij B_ij; {2,1}/{2,1} = A_ji B_ji = same sum
        return static_cast<ValueType>(tmech::dcontract(l, r));
      }
      // {1,2}/{2,1} (either orientation) = A_ij B_ji
      return static_cast<ValueType>(tmech::dcontract(l, tmech::trans(r)));
    } else if constexpr (Rank == 1) {
      using Tensor = tensor_data<ValueType, Dim, Rank>;
      auto const &l = static_cast<const Tensor &>(m_lhs).data();
      auto const &r = static_cast<const Tensor &>(m_rhs).data();
      return static_cast<ValueType>(tmech::dot(l, r));
    } else {
      throw evaluation_error(
          "tensor_data_dcontract_wrapper: rank > 2 contraction not "
          "implemented (#353 follow-up in #383)");
    }
  }

  ValueType mismatch(std::size_t dim, std::size_t rank) {
    if (dim > this->MaxDim_ || dim == 0)
      throw evaluation_error(
          "tensor_data_dcontract_wrapper: dim > MaxDim || dim == 0");
    if (rank > this->MaxRank_ || rank == 0)
      throw evaluation_error(
          "tensor_data_dcontract_wrapper: rank > MaxRank || rank == 0");
    return ValueType{};
  }

private:
  tensor_data_base<ValueType> const &m_lhs;
  tensor_data_base<ValueType> const &m_rhs;
  sequence m_lhs_indices;
  sequence m_rhs_indices;
};

// ─── Eigenvalue wrapper: dispatches runtime (dim,rank), computes the
//     ascending-sorted eigenvalues of a symmetric rank-2 tensor and
//     returns the index-th one (#226) ────────────────────────────────────
//
template <typename ValueType>
class tensor_data_eigenvalue_wrapper final
    : public tensor_data_eval_up_unary<
          tensor_data_eigenvalue_wrapper<ValueType>, ValueType> {
public:
  tensor_data_eigenvalue_wrapper(tensor_data_base<ValueType> const &input,
                                 std::size_t index)
      : m_input(input), m_index(index) {}

  template <std::size_t Dim, std::size_t Rank> ValueType evaluate_imp() {
    if constexpr (Rank == 2 && (Dim == 2 || Dim == 3)) {
      if (m_index >= Dim)
        throw evaluation_error(
            "tensor_data_eigenvalue_wrapper: eigenvalue index out of range");
      using Tensor = tensor_data<ValueType, Dim, Rank>;
      auto const &in = static_cast<const Tensor &>(m_input).data();
      return spectral::cached_decompose<ValueType, Dim>(in)
          .eigenvalues[m_index];
    } else {
      throw evaluation_error("tensor_data_eigenvalue_wrapper: requires a "
                             "rank-2 tensor of dim 2 or 3");
    }
  }

  ValueType mismatch(std::size_t dim, std::size_t rank) {
    if (dim > this->MaxDim_ || dim == 0)
      throw evaluation_error(
          "tensor_data_eigenvalue_wrapper: dim > MaxDim || dim == 0");
    if (rank > this->MaxRank_ || rank == 0)
      throw evaluation_error(
          "tensor_data_eigenvalue_wrapper: rank > MaxRank || rank == 0");
    return ValueType{};
  }

private:
  tensor_data_base<ValueType> const &m_input;
  std::size_t m_index;
};

// ─── tmech scalar-returning operation policies ──────────────────────────

namespace tmech_ops {

struct trace_op {
  template <std::size_t, std::size_t Rank> static constexpr bool is_valid() {
    return Rank == 2;
  }
  template <typename T> static constexpr auto apply(T const &t) {
    return tmech::trace(t);
  }
};

struct det_op {
  template <std::size_t Dim, std::size_t Rank>
  static constexpr bool is_valid() {
    return Rank == 2 && (Dim == 2 || Dim == 3);
  }
  template <typename T> static constexpr auto apply(T const &t) {
    return tmech::det(t);
  }
};

struct norm_op {
  template <std::size_t, std::size_t Rank> static constexpr bool is_valid() {
    return Rank == 2;
  }
  template <typename T> static constexpr auto apply(T const &t) {
    return tmech::norm(t);
  }
};

struct dcontract_self_op {
  template <std::size_t, std::size_t Rank> static constexpr bool is_valid() {
    return Rank == 2;
  }
  template <typename T> static constexpr auto apply(T const &t) {
    return tmech::dcontract(t, t);
  }
};

} // namespace tmech_ops

} // namespace numsim::cas

#endif // TENSOR_DATA_TO_SCALAR_WRAPPER_H
