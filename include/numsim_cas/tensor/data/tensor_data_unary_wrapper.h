#ifndef TENSOR_DATA_UNARY_WRAPPER_H
#define TENSOR_DATA_UNARY_WRAPPER_H

#include "tensor_data.h"
#include <numsim_cas/core/cas_error.h>

namespace numsim::cas {

// ─── Generic unary wrapper: dispatches runtime (dim,rank) to tmech Op ────────
//
// Op must provide:
//   static constexpr bool is_valid<Dim, Rank>()   — compile-time guard
//   static auto apply(tmech_tensor const&)         — the tmech call
//
template <typename Op, typename ValueType>
class tensor_data_unary_wrapper final
    : public tensor_data_eval_up_unary<tensor_data_unary_wrapper<Op, ValueType>,
                                       ValueType> {
public:
  tensor_data_unary_wrapper(tensor_data_base<ValueType> &result,
                            tensor_data_base<ValueType> const &input)
      : m_result(result), m_input(input) {}

  template <std::size_t Dim, std::size_t Rank> void evaluate_imp() {
    if constexpr (Op::template is_valid<Dim, Rank>()) {
      using Tensor = tensor_data<ValueType, Dim, Rank>;
      auto const &in = static_cast<const Tensor &>(m_input).data();
      static_cast<Tensor &>(m_result).data() = Op::apply(in);
    } else {
      throw evaluation_error("tensor_data_unary_wrapper: invalid dim/rank");
    }
  }

  void missmatch(std::size_t dim, std::size_t rank) {
    if (dim > this->_MaxDim || dim == 0)
      throw evaluation_error(
          "tensor_data_unary_wrapper: dim > MaxDim || dim == 0");
    if (rank > this->_MaxRank || rank == 0)
      throw evaluation_error(
          "tensor_data_unary_wrapper: rank > MaxRank || rank == 0");
  }

private:
  tensor_data_base<ValueType> &m_result;
  tensor_data_base<ValueType> const &m_input;
};

// ─── Identity tensor creation via tmech::eye ─────────────────────────────────

template <typename ValueType>
class tensor_data_identity final
    : public tensor_data_eval_up_unary<tensor_data_identity<ValueType>,
                                       ValueType> {
public:
  explicit tensor_data_identity(tensor_data_base<ValueType> &result)
      : m_result(result) {}

  template <std::size_t Dim, std::size_t Rank> void evaluate_imp() {
    if constexpr (Rank % 2 == 0) {
      using Tensor = tensor_data<ValueType, Dim, Rank>;
      static_cast<Tensor &>(m_result).data() =
          tmech::eye<ValueType, Dim, Rank>();
    } else {
      throw evaluation_error(
          "tensor_data_identity: identity requires even rank");
    }
  }

  void missmatch(std::size_t dim, std::size_t rank) {
    if (dim > this->_MaxDim || dim == 0)
      throw evaluation_error(
          "tensor_data_identity: dim > MaxDim || dim == 0");
    if (rank > this->_MaxRank || rank == 0)
      throw evaluation_error(
          "tensor_data_identity: rank > MaxRank || rank == 0");
  }

private:
  tensor_data_base<ValueType> &m_result;
};

// ─── tmech operation policy structs ──────────────────────────────────────────

namespace tmech_ops {

struct sym {
  template <std::size_t, std::size_t Rank>
  static constexpr bool is_valid() {
    return Rank == 2;
  }
  template <typename T> static constexpr auto apply(T const &t) {
    return tmech::sym(t);
  }
};

struct dev {
  template <std::size_t, std::size_t Rank>
  static constexpr bool is_valid() {
    return Rank == 2;
  }
  template <typename T> static constexpr auto apply(T const &t) {
    return tmech::dev(t);
  }
};

struct vol {
  template <std::size_t, std::size_t Rank>
  static constexpr bool is_valid() {
    return Rank == 2;
  }
  template <typename T> static constexpr auto apply(T const &t) {
    return tmech::vol(t);
  }
};

struct inv {
  template <std::size_t Dim, std::size_t Rank>
  static constexpr bool is_valid() {
    return (Rank == 2 || Rank == 4) && (Dim == 2 || Dim == 3);
  }
  template <typename T> static constexpr auto apply(T const &t) {
    return tmech::inv(t);
  }
};

struct invf {
  template <std::size_t Dim, std::size_t Rank>
  static constexpr bool is_valid() {
    return (Rank == 2 || Rank == 4) && (Dim == 2 || Dim == 3);
  }
  template <typename T> static constexpr auto apply(T const &t) {
    return tmech::invf(t);
  }
};

struct adj {
  template <std::size_t Dim, std::size_t Rank>
  static constexpr bool is_valid() {
    return Rank == 2 && (Dim == 2 || Dim == 3);
  }
  template <typename T> static constexpr auto apply(T const &t) {
    return tmech::adj(t);
  }
};

struct cof {
  template <std::size_t Dim, std::size_t Rank>
  static constexpr bool is_valid() {
    return Rank == 2 && (Dim == 2 || Dim == 3);
  }
  template <typename T> static constexpr auto apply(T const &t) {
    return tmech::cof(t);
  }
};

struct neg {
  template <std::size_t, std::size_t> static constexpr bool is_valid() {
    return true;
  }
  template <typename T> static constexpr auto apply(T const &t) { return -t; }
};

} // namespace tmech_ops

} // namespace numsim::cas

#endif // TENSOR_DATA_UNARY_WRAPPER_H
