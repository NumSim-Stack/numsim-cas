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

// Single cache slot. A hit requires the hash and the raw components to match
// bitwise, so a hash collision recomputes instead of returning another
// tensor's decomposition (-0.0 and 0.0 are distinct keys, as in the hash).
template <typename ValueType, std::size_t Dim> struct decomposition_slot {
  decomposition<ValueType, Dim> value{};
  tmech::tensor<ValueType, Dim, 2> components{};
  std::size_t key = 0;
  bool valid = false;

  bool hit(std::size_t in_key,
           tmech::tensor<ValueType, Dim, 2> const &in) const {
    return valid && key == in_key &&
           std::memcmp(components.raw_data(), in.raw_data(),
                       sizeof(ValueType) * Dim * Dim) == 0;
  }
};

} // namespace detail

// Eigendecomposition of sym(A), ascending, cached by content. A single-entry
// thread-local cache (one per ValueType/Dim) shared by every spectral wrapper,
// so value(i)/basis(i)/normal(i) and the isotropic-function wrappers decompose
// the same tensor once. Interleaving two different tensors thrashes the slot.
template <typename ValueType, std::size_t Dim>
decomposition<ValueType, Dim> const &
cached_decompose(tmech::tensor<ValueType, Dim, 2> const &in) {
  static thread_local detail::decomposition_slot<ValueType, Dim> slot;

  const std::size_t key = detail::content_hash(in);
  if (slot.hit(key, in))
    return slot.value;

  auto decomp = tmech::eigen_decomposition(tmech::sym(in));
  auto const [eigvals, eigvecs] = decomp.decompose();

  std::array<std::size_t, Dim> order{};
  for (std::size_t i = 0; i < Dim; ++i)
    order[i] = i;
  std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
    return eigvals[a] < eigvals[b];
  });

  for (std::size_t i = 0; i < Dim; ++i) {
    slot.value.eigenvalues[i] = eigvals[order[i]];
    slot.value.eigenvectors[i] = eigvecs[order[i]];
  }
  slot.components = in;
  slot.key = key;
  slot.valid = true;
  return slot.value;
}

} // namespace numsim::cas::spectral

#endif // NUMSIM_CAS_SPECTRAL_DECOMPOSITION_CACHE_H
