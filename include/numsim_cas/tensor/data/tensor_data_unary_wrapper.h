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
    if (dim > this->MaxDim_ || dim == 0)
      throw evaluation_error(
          "tensor_data_unary_wrapper: dim > MaxDim || dim == 0");
    if (rank > this->MaxRank_ || rank == 0)
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
          for (std::size_t i = 0; i < Rank; ++i)
            s *= Dim;
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
            if (indices[k] != indices[R + k]) {
              val = 0;
              break;
            }
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
    if (dim > this->MaxDim_ || dim == 0)
      throw evaluation_error("tensor_data_identity: dim > MaxDim || dim == 0");
    if (rank > this->MaxRank_ || rank == 0)
      throw evaluation_error(
          "tensor_data_identity: rank > MaxRank || rank == 0");
  }

private:
  tensor_data_base<ValueType> &m_result;
};

// ─── Levi-Civita tensor creation via parity counting ─────────────────────────
//
// Populates a `tensor_data<ValueType, Dim, Dim>` with the permutation symbol
// ε_{i₁…i_Dim}. Rank is always equal to Dim (the LC symbol's rank is its
// dimension by definition).
//
// We compute components directly from permutation parity rather than going
// through `tmech::levi_civita`. The reason: tmech's 3D `operator()` uses
// the formula `(j-i)(k-j)(i-k)/2`, which evaluates to −1 for the identity
// permutation (i,j,k)=(0,1,2) instead of the +1 required by the standard
// sign convention (the one for which det(I) = ε_{ijk} I_{1i} I_{2j} I_{3k}
// yields +1). The dim-2 and dim-4 tmech formulae are consistent with the
// standard convention, but the dim-3 sign flip would silently break
// downstream uses (cross product, determinant, curl). A direct parity
// implementation is also no harder to read and is independent of any
// future tmech changes.
//
// **Evaluation domain.** The framework's `tensor_data_eval_up_unary` is
// instantiated at `MaxDim = 3` (see `numsim_cas_type_traits.h`), so
// `evaluate(dim=4, rank=4)` triggers the `mismatch()` path. Construction
// of `levi_civita_tensor(4)` is still allowed at the symbolic layer — the
// dim-4 case is useful inside expressions that ultimately collapse before
// numerical evaluation — but `tensor_evaluator::apply` on a bare
// `levi_civita_tensor(4)` will throw. Lifting that ceiling is part of the
// broader MaxDim-bump effort tracked elsewhere.
template <typename ValueType>
class tensor_data_levi_civita final
    : public tensor_data_eval_up_unary<tensor_data_levi_civita<ValueType>,
                                       ValueType> {
public:
  explicit tensor_data_levi_civita(tensor_data_base<ValueType> &result)
      : m_result(result) {}

  template <std::size_t Dim, std::size_t Rank> void evaluate_imp() {
    if constexpr (Dim == Rank && (Dim == 2 || Dim == 3 || Dim == 4)) {
      using Tensor = tensor_data<ValueType, Dim, Rank>;
      auto &data = static_cast<Tensor &>(m_result).data();
      constexpr std::size_t total = []() {
        std::size_t s = 1;
        for (std::size_t i = 0; i < Rank; ++i)
          s *= Dim;
        return s;
      }();
      auto *raw = data.raw_data();
      for (std::size_t idx = 0; idx < total; ++idx) {
        std::size_t tmp = idx;
        std::size_t indices[Rank];
        for (std::size_t k = Rank; k > 0; --k) {
          indices[k - 1] = tmp % Dim;
          tmp /= Dim;
        }
        raw[idx] = parity_sign(indices);
      }
    } else {
      throw evaluation_error("tensor_data_levi_civita: only Dim == Rank with "
                             "Dim ∈ {2, 3, 4} are supported");
    }
  }

  void mismatch(std::size_t dim, std::size_t rank) {
    if (dim != rank) {
      throw evaluation_error(
          "tensor_data_levi_civita: rank must equal dim (LC at dim N is "
          "rank-N by definition)");
    }
    if (dim < 2 || dim > 4) {
      throw evaluation_error(
          "tensor_data_levi_civita: only Dim ∈ {2, 3, 4} are supported");
    }
  }

private:
  // Computes ε_{indices} by bubble-sort parity counting.
  // Returns 0 if any index repeats (the permutation is degenerate);
  // otherwise +1 for an even permutation and -1 for an odd one.
  // The choice of bubble sort is O(N²) but `Rank` is bounded by 4
  // here — the work is negligible.
  template <std::size_t Rank>
  static constexpr ValueType parity_sign(std::size_t (&indices)[Rank]) {
    std::size_t idx[Rank];
    for (std::size_t i = 0; i < Rank; ++i) {
      idx[i] = indices[i];
    }
    for (std::size_t i = 0; i < Rank; ++i) {
      for (std::size_t j = i + 1; j < Rank; ++j) {
        if (idx[i] == idx[j])
          return ValueType{0};
      }
    }
    std::size_t swaps = 0;
    for (std::size_t i = 0; i + 1 < Rank; ++i) {
      for (std::size_t j = i + 1; j < Rank; ++j) {
        if (idx[i] > idx[j]) {
          std::swap(idx[i], idx[j]);
          ++swaps;
        }
      }
    }
    return (swaps % 2 == 0) ? ValueType{1} : ValueType{-1};
  }

  tensor_data_base<ValueType> &m_result;
};

// ─── tmech operation policy structs ──────────────────────────────────────────

namespace tmech_ops {

struct sym {
  template <std::size_t, std::size_t Rank> static constexpr bool is_valid() {
    return Rank == 2;
  }
  template <typename T> static constexpr auto apply(T const &t) {
    return tmech::sym(t);
  }
};

struct dev {
  template <std::size_t, std::size_t Rank> static constexpr bool is_valid() {
    return Rank == 2;
  }
  template <typename T> static constexpr auto apply(T const &t) {
    // P_dev is the symmetric-deviatoric projector: P_dev : X = sym(X) - vol(X).
    // tmech::dev(X) = X - vol(X) which only matches for symmetric X.
    // Apply symmetrization first to match the projector semantics.
    return tmech::dev(tmech::sym(t));
  }
};

struct vol {
  template <std::size_t, std::size_t Rank> static constexpr bool is_valid() {
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
  template <std::size_t, std::size_t Rank> static constexpr bool is_valid() {
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
