#ifndef NUMSIM_CAS_TENSOR_DATA_ISOTROPIC_H
#define NUMSIM_CAS_TENSOR_DATA_ISOTROPIC_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

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

// The n-th derivative f^{(n)}(x). Used by the confluent divided difference
// for a repeated point.
template <typename V> V apply_fderiv(isotropic_kind k, V x, std::size_t n) {
  switch (k) {
  case isotropic_kind::exp:
    return std::exp(x); // all orders
  case isotropic_kind::log: {
    if (n == 0)
      return std::log(x);
    // f^{(n)} = (-1)^{n-1} (n-1)! / x^n
    V fact{1};
    for (std::size_t j = 2; j < n; ++j)
      fact *= static_cast<V>(j);
    const V sign = ((n - 1) % 2 == 0) ? V{1} : V{-1};
    return sign * fact / std::pow(x, static_cast<V>(n));
  }
  case isotropic_kind::sqrt: {
    if (n == 0)
      return std::sqrt(x);
    // f^{(n)} = (1/2)(1/2-1)...(1/2-n+1) x^{1/2-n}
    V coef{1};
    for (std::size_t j = 0; j < n; ++j)
      coef *= (V{0.5} - static_cast<V>(j));
    return coef * std::pow(x, V{0.5} - static_cast<V>(n));
  }
  }
  return V{};
}

// Confluent (Hermite) divided difference [f; p_lo, ..., p_hi] over the
// ascending-sorted points p. A span whose endpoints are equal (within a
// relative band — hence every point between them is too) collapses to the
// derivative form f^{(hi-lo)}(p_lo)/(hi-lo)!, so coincident eigenvalues give
// the analytic limit instead of 0/0.
template <typename V>
V dd_range(isotropic_kind k, V const *p, std::size_t lo, std::size_t hi,
           V rel) {
  const V span = p[hi] - p[lo];
  const V scale = std::max(std::abs(p[lo]), std::abs(p[hi]));
  if (span == V{0} || std::abs(span) <= rel * scale) {
    V fact{1};
    for (std::size_t j = 2; j <= hi - lo; ++j)
      fact *= static_cast<V>(j);
    return apply_fderiv(k, p[lo], hi - lo) / fact;
  }
  return (dd_range(k, p, lo + 1, hi, rel) - dd_range(k, p, lo, hi - 1, rel)) /
         span;
}

template <typename V> V confluent_dd(isotropic_kind k, std::vector<V> points) {
  std::sort(points.begin(), points.end());
  const V rel = std::sqrt(std::numeric_limits<V>::epsilon());
  return dd_range(k, points.data(), std::size_t{0}, points.size() - 1, rel);
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

// [f; λ_{i0}, ..., λ_{ik}] — the confluent divided difference of f over a
// multiset of eigenvalues of a symmetric rank-2 tensor (#326). Scalar-valued.
// The building block of the symbolic spectral tangent: it evaluates safely at
// coincident eigenvalues and differentiates to a higher divided difference,
// so the isotropic tensor function is differentiable to any order.
template <typename ValueType>
class tensor_data_divided_difference_wrapper final
    : public tensor_data_eval_up_unary<
          tensor_data_divided_difference_wrapper<ValueType>, ValueType> {
public:
  tensor_data_divided_difference_wrapper(
      tensor_data_base<ValueType> const &input, isotropic_kind kind,
      std::vector<std::size_t> const &indices)
      : m_input(input), m_kind(kind), m_indices(indices) {}

  template <std::size_t Dim, std::size_t Rank> ValueType evaluate_imp() {
    if constexpr (Rank == 2 && (Dim == 2 || Dim == 3)) {
      using Tensor = tensor_data<ValueType, Dim, Rank>;
      auto const &in = static_cast<const Tensor &>(m_input).data();
      std::array<ValueType, Dim> lam{};
      std::array<tmech::tensor<ValueType, Dim, 1>, Dim> vec{};
      iso_detail::decompose_sorted(in, lam, vec);
      std::vector<ValueType> points;
      points.reserve(m_indices.size());
      for (std::size_t idx : m_indices) {
        if (idx >= Dim)
          throw evaluation_error(
              "tensor_data_divided_difference_wrapper: index out of range");
        points.push_back(lam[idx]);
      }
      return iso_detail::confluent_dd(m_kind, std::move(points));
    } else {
      throw evaluation_error(
          "tensor_data_divided_difference_wrapper: requires rank-2 dim 2/3");
    }
  }

  ValueType mismatch(std::size_t dim, std::size_t rank) {
    if (dim > this->MaxDim_ || dim == 0 || rank > this->MaxRank_ || rank == 0)
      throw evaluation_error(
          "tensor_data_divided_difference_wrapper: bad dim/rank");
    return ValueType{};
  }

private:
  tensor_data_base<ValueType> const &m_input;
  isotropic_kind m_kind;
  std::vector<std::size_t> m_indices;
};

} // namespace numsim::cas

#endif // NUMSIM_CAS_TENSOR_DATA_ISOTROPIC_H
