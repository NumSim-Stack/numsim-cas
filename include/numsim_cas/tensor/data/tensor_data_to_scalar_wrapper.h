#ifndef TENSOR_DATA_TO_SCALAR_WRAPPER_H
#define TENSOR_DATA_TO_SCALAR_WRAPPER_H

#include "tensor_data.h"
#include <numsim_cas/core/cas_error.h>

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
      throw evaluation_error(
          "tensor_data_to_scalar_wrapper: invalid dim/rank");
    }
  }

  ValueType missmatch(std::size_t dim, std::size_t rank) {
    if (dim > this->_MaxDim || dim == 0)
      throw evaluation_error(
          "tensor_data_to_scalar_wrapper: dim > MaxDim || dim == 0");
    if (rank > this->_MaxRank || rank == 0)
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
    : public tensor_data_eval_up_unary<
          tensor_data_dcontract_wrapper<ValueType>, ValueType> {
public:
  tensor_data_dcontract_wrapper(tensor_data_base<ValueType> const &lhs,
                                tensor_data_base<ValueType> const &rhs)
      : m_lhs(lhs), m_rhs(rhs) {}

  template <std::size_t Dim, std::size_t Rank> ValueType evaluate_imp() {
    if constexpr (Rank == 2) {
      using Tensor = tensor_data<ValueType, Dim, Rank>;
      auto const &l = static_cast<const Tensor &>(m_lhs).data();
      auto const &r = static_cast<const Tensor &>(m_rhs).data();
      return static_cast<ValueType>(tmech::dcontract(l, r));
    } else {
      throw evaluation_error(
          "tensor_data_dcontract_wrapper: requires rank 2");
    }
  }

  ValueType missmatch(std::size_t dim, std::size_t rank) {
    if (dim > this->_MaxDim || dim == 0)
      throw evaluation_error(
          "tensor_data_dcontract_wrapper: dim > MaxDim || dim == 0");
    if (rank > this->_MaxRank || rank == 0)
      throw evaluation_error(
          "tensor_data_dcontract_wrapper: rank > MaxRank || rank == 0");
    return ValueType{};
  }

private:
  tensor_data_base<ValueType> const &m_lhs;
  tensor_data_base<ValueType> const &m_rhs;
};

// ─── tmech scalar-returning operation policies ──────────────────────────

namespace tmech_ops {

struct trace_op {
  template <std::size_t, std::size_t Rank>
  static constexpr bool is_valid() {
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
  template <std::size_t, std::size_t Rank>
  static constexpr bool is_valid() {
    return Rank == 2;
  }
  template <typename T> static constexpr auto apply(T const &t) {
    return tmech::norm(t);
  }
};

struct dcontract_self_op {
  template <std::size_t, std::size_t Rank>
  static constexpr bool is_valid() {
    return Rank == 2;
  }
  template <typename T> static constexpr auto apply(T const &t) {
    return tmech::dcontract(t, t);
  }
};

} // namespace tmech_ops

} // namespace numsim::cas

#endif // TENSOR_DATA_TO_SCALAR_WRAPPER_H
