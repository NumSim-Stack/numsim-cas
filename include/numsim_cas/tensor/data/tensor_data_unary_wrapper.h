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

  void mismatch(std::size_t dim, std::size_t rank) {
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
      auto &data = static_cast<Tensor &>(m_result).data();
      if constexpr (Rank == 2) {
        data = tmech::eye<ValueType, Dim, 2>();
      } else if constexpr (Rank == 4) {
        // Minor identity: I_{ijkl} = delta_ik * delta_jl
        // This is the 4th-order identity for differentiation: dA_ij/dA_kl
        auto I2 = tmech::eye<ValueType, Dim, 2>();
        data = tmech::otimesu(I2, I2);
      } else {
        // General minor identity for rank 2R:
        // I_{i0,...,iR-1,j0,...,jR-1} = prod_k delta(i_k, j_{k})
        constexpr std::size_t R = Rank / 2;
        constexpr std::size_t total = []() {
          std::size_t s = 1;
          for (std::size_t i = 0; i < Rank; ++i) s *= Dim;
          return s;
        }();
        auto *raw = data.raw_data();
        for (std::size_t idx = 0; idx < total; ++idx) {
          // Decompose flat index into multi-index
          std::size_t tmp = idx;
          std::size_t indices[Rank];
          for (std::size_t k = Rank; k > 0; --k) {
            indices[k - 1] = tmp % Dim;
            tmp /= Dim;
          }
          // Check if i_k == i_{R+k} for all k in [0, R)
          ValueType val{1};
          for (std::size_t k = 0; k < R; ++k) {
            if (indices[k] != indices[R + k]) { val = 0; break; }
          }
          raw[idx] = val;
        }
      }
    } else {
      throw evaluation_error(
          "tensor_data_identity: identity requires even rank");
    }
  }

  void mismatch(std::size_t dim, std::size_t rank) {
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

struct skew {
  template <std::size_t, std::size_t Rank>
  static constexpr bool is_valid() {
    return Rank == 2;
  }
  template <typename T> static constexpr auto apply(T const &t) {
    return tmech::skew(t);
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
