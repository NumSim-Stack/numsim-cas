#ifndef NUMSIM_CAS_TENSOR_DATA_ISOTROPIC_H
#define NUMSIM_CAS_TENSOR_DATA_ISOTROPIC_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

#include "tensor_data.h"
#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/numsim_cas_type_traits.h>
#include <numsim_cas/tensor/isotropic_kind.h>

namespace numsim::cas {

namespace iso_detail {

template <typename V> V apply_f(isotropic_kind k, V x) {
  switch (k) {
  case isotropic_kind::log:
    return std::log(x);
  case isotropic_kind::exp:
    return std::exp(x);
  case isotropic_kind::sqrt:
    return std::sqrt(x);
  }
  return V{};
}

template <typename V> V apply_fprime(isotropic_kind k, V x) {
  switch (k) {
  case isotropic_kind::log:
    return V{1} / x;
  case isotropic_kind::exp:
    return std::exp(x);
  case isotropic_kind::sqrt:
    return V{1} / (V{2} * std::sqrt(x));
  }
  return V{};
}

// Decompose sym(in) into ascending eigenvalues and matching eigenvectors.
template <typename V, std::size_t Dim>
void decompose_sorted(tmech::tensor<V, Dim, 2> const &in,
                      std::array<V, Dim> &lam,
                      std::array<tmech::tensor<V, Dim, 1>, Dim> &vec) {
  auto decomp = tmech::eigen_decomposition(tmech::sym(in));
  auto const [eigvals, eigvecs] = decomp.decompose();
  std::array<std::size_t, Dim> order{};
  for (std::size_t i = 0; i < Dim; ++i)
    order[i] = i;
  std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
    return eigvals[a] < eigvals[b];
  });
  for (std::size_t i = 0; i < Dim; ++i) {
    lam[i] = eigvals[order[i]];
    vec[i] = eigvecs[order[i]];
  }
}

} // namespace iso_detail

// f(A) = Σ_i f(λ_i) E_i for a symmetric rank-2 tensor (#227). Basis-invariant,
// so robust at coincident eigenvalues.
template <typename ValueType>
class tensor_data_isotropic_value_wrapper final
    : public tensor_data_eval_up_unary<
          tensor_data_isotropic_value_wrapper<ValueType>, ValueType> {
public:
  tensor_data_isotropic_value_wrapper(tensor_data_base<ValueType> &result,
                                      tensor_data_base<ValueType> const &input,
                                      isotropic_kind kind)
      : m_result(result), m_input(input), m_kind(kind) {}

  template <std::size_t Dim, std::size_t Rank> void evaluate_imp() {
    if constexpr (Rank == 2 && (Dim == 2 || Dim == 3)) {
      using Tensor = tensor_data<ValueType, Dim, Rank>;
      auto const &in = static_cast<const Tensor &>(m_input).data();
      std::array<ValueType, Dim> lam{};
      std::array<tmech::tensor<ValueType, Dim, 1>, Dim> vec{};
      iso_detail::decompose_sorted(in, lam, vec);
      tmech::tensor<ValueType, Dim, 2> out;
      for (std::size_t i = 0; i < Dim; ++i)
        out +=
            iso_detail::apply_f(m_kind, lam[i]) * tmech::otimes(vec[i], vec[i]);
      static_cast<Tensor &>(m_result).data() = out;
    } else {
      throw evaluation_error(
          "tensor_data_isotropic_value_wrapper: requires rank-2 dim 2/3");
    }
  }

  void mismatch(std::size_t dim, std::size_t rank) {
    if (dim > this->MaxDim_ || dim == 0 || rank > this->MaxRank_ || rank == 0)
      throw evaluation_error(
          "tensor_data_isotropic_value_wrapper: bad dim/rank");
  }

private:
  tensor_data_base<ValueType> &m_result;
  tensor_data_base<ValueType> const &m_input;
  isotropic_kind m_kind;
};

// ∂f(A)/∂A, the coincidence-safe Daleckii–Krein tangent (#326):
//
//   ∂f/∂A = Σ_i f'(λ_i) E_i⊗E_i
//         + Σ_{i<j} dd_ij ( E_i⊙E_j + E_j⊙E_i ),
//   dd_ij = |λ_i−λ_j| < tol ? f'(λ_i) : (f(λ_i)−f(λ_j))/(λ_i−λ_j),
//   X⊙Y = ½(otimesu + otimesl).
//
// The divided-difference guard removes the 1/(λ_i−λ_j) singularity: at
// coincidence the coefficient is the analytic limit f'(λ_i), so the tangent
// stays finite (and correct) through eigenvalue coalescence.
template <typename ValueType>
class tensor_data_isotropic_tangent_wrapper final
    : public tensor_data_eval_up_unary<
          tensor_data_isotropic_tangent_wrapper<ValueType>, ValueType> {
public:
  tensor_data_isotropic_tangent_wrapper(
      tensor_data_base<ValueType> &result,
      tensor_data_base<ValueType> const &input, isotropic_kind kind)
      : m_result(result), m_input(input), m_kind(kind) {}

  template <std::size_t Dim, std::size_t Rank> void evaluate_imp() {
    // Output is rank-4; dispatched on the result's rank.
    if constexpr (Rank == 4 && (Dim == 2 || Dim == 3)) {
      using InTensor = tensor_data<ValueType, Dim, 2>;
      using OutTensor = tensor_data<ValueType, Dim, 4>;
      auto const &in = static_cast<const InTensor &>(m_input).data();
      std::array<ValueType, Dim> lam{};
      std::array<tmech::tensor<ValueType, Dim, 1>, Dim> vec{};
      iso_detail::decompose_sorted(in, lam, vec);

      std::array<tmech::tensor<ValueType, Dim, 2>, Dim> E{};
      for (std::size_t i = 0; i < Dim; ++i)
        E[i] = tmech::otimes(vec[i], vec[i]);

      const ValueType rel =
          std::sqrt(std::numeric_limits<ValueType>::epsilon());

      tmech::tensor<ValueType, Dim, 4> out;
      for (std::size_t i = 0; i < Dim; ++i)
        out += iso_detail::apply_fprime(m_kind, lam[i]) *
               tmech::otimes(E[i], E[i]);
      for (std::size_t i = 0; i < Dim; ++i) {
        for (std::size_t j = i + 1; j < Dim; ++j) {
          const ValueType diff = lam[i] - lam[j];
          // Switch to the analytic limit f'(λ) once the pair is closer than
          // sqrt(eps) RELATIVE to its own magnitude — that is where a
          // repeated root loses accuracy and its eigenvectors go
          // ill-conditioned. A per-pair relative band (not a global one)
          // avoids falsely merging two genuinely-distinct small eigenvalues,
          // whose divided difference is large but correct (e.g. log at small
          // λ). `diff == 0` also captures an exactly-repeated pair of zeros.
          const ValueType tol =
              rel * std::max(std::abs(lam[i]), std::abs(lam[j]));
          const bool coincident = diff == ValueType{0} || std::abs(diff) <= tol;
          const ValueType dd = coincident
                                   ? iso_detail::apply_fprime(m_kind, lam[i])
                                   : (iso_detail::apply_f(m_kind, lam[i]) -
                                      iso_detail::apply_f(m_kind, lam[j])) /
                                         diff;
          out += (ValueType{0.5} * dd) *
                 (tmech::otimesu(E[i], E[j]) + tmech::otimesl(E[i], E[j]) +
                  tmech::otimesu(E[j], E[i]) + tmech::otimesl(E[j], E[i]));
        }
      }
      static_cast<OutTensor &>(m_result).data() = out;
    } else {
      throw evaluation_error(
          "tensor_data_isotropic_tangent_wrapper: requires rank-2 dim 2/3");
    }
  }

  void mismatch(std::size_t dim, std::size_t rank) {
    if (dim > this->MaxDim_ || dim == 0 || rank > this->MaxRank_ || rank == 0)
      throw evaluation_error(
          "tensor_data_isotropic_tangent_wrapper: bad dim/rank");
  }

private:
  tensor_data_base<ValueType> &m_result;
  tensor_data_base<ValueType> const &m_input;
  isotropic_kind m_kind;
};

} // namespace numsim::cas

#endif // NUMSIM_CAS_TENSOR_DATA_ISOTROPIC_H
