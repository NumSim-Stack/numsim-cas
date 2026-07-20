#ifndef NUMSIM_CAS_SPECTRAL_DECOMPOSITION_CACHE_H
#define NUMSIM_CAS_SPECTRAL_DECOMPOSITION_CACHE_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "tensor_data.h"

namespace numsim::cas::spectral {

// Ascending-sorted eigenvalues and matching eigenvectors of a symmetric
// rank-2 tensor. Populated once per distinct input and shared by the
// eigenvalue / eigenprojection / eigenvector wrappers (#325).
template <typename ValueType, std::size_t Dim> struct decomposition {
  std::array<ValueType, Dim> eigenvalues{};
  std::array<tmech::tensor<ValueType, Dim, 1>, Dim> eigenvectors{};
};

namespace detail {

// FNV-1a over the tensor's raw components — a content key so the same
// numeric tensor reuses its decomposition regardless of which node
// requested it.
template <typename ValueType, std::size_t Dim>
inline std::size_t content_hash(tmech::tensor<ValueType, Dim, 2> const &t) {
  auto const *data = t.raw_data();
  std::uint64_t h = 1469598103934665603ULL;
  for (std::size_t i = 0; i < Dim * Dim; ++i) {
    std::uint64_t bits = 0;
    std::memcpy(&bits, &data[i],
                sizeof(ValueType) <= sizeof(bits) ? sizeof(ValueType)
                                                  : sizeof(bits));
    h ^= bits;
    h *= 1099511628211ULL;
  }
  return static_cast<std::size_t>(h);
}

} // namespace detail

// Eigendecomposition of sym(A), ascending, cached by content. A single-entry
// thread-local cache (one per ValueType/Dim) — it collapses the repeated
// decomposition of the *same* tensor that value(i)/basis(i)/normal(i)
// otherwise trigger (a spectral stress decomposed A once per spectral
// quantity before this). Correct by construction: the key is the tensor's
// contents, so a hit returns the decomposition of exactly that tensor; a
// changed tensor misses and recomputes. Interleaving two different tensors
// simply thrashes the single slot — never wrong, only unshared.
template <typename ValueType, std::size_t Dim>
decomposition<ValueType, Dim> const &
cached_decompose(tmech::tensor<ValueType, Dim, 2> const &in) {
  static thread_local decomposition<ValueType, Dim> cache;
  static thread_local std::size_t cached_key = 0;
  static thread_local bool cached_valid = false;

  const std::size_t key = detail::content_hash(in);
  if (cached_valid && cached_key == key)
    return cache;

  auto decomp = tmech::eigen_decomposition(tmech::sym(in));
  auto const [eigvals, eigvecs] = decomp.decompose();

  std::array<std::size_t, Dim> order{};
  for (std::size_t i = 0; i < Dim; ++i)
    order[i] = i;
  std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
    return eigvals[a] < eigvals[b];
  });

  for (std::size_t i = 0; i < Dim; ++i) {
    cache.eigenvalues[i] = eigvals[order[i]];
    cache.eigenvectors[i] = eigvecs[order[i]];
  }
  cached_key = key;
  cached_valid = true;
  return cache;
}

} // namespace numsim::cas::spectral

#endif // NUMSIM_CAS_SPECTRAL_DECOMPOSITION_CACHE_H
