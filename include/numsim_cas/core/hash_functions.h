#ifndef HASH_FUNCTIONS_H
#define HASH_FUNCTIONS_H

#include <bit>
#include <complex>
#include <cstdint>
#include <string>
#include <vector>

namespace numsim::cas {

// Helper function for combining hashes
template <typename T>
inline void hash_combine(std::size_t &seed, const T &value) {
  // std::hash<T> hasher;
  seed ^= static_cast<std::size_t>(value) +
          static_cast<std::size_t>(0x9e3779b9) + (seed << 6) + (seed >> 2);
}

// Doubles hash by bit pattern: the generic static_cast is UB for negative
// values and collapses all fractions in (0,1) onto 0 (#361).
inline void hash_combine(std::size_t &seed, double value) {
  if (value == 0.0)
    value = 0.0; // normalize -0.0
  hash_combine(seed, std::bit_cast<std::uint64_t>(value));
}

inline void hash_combine(std::size_t &seed, float value) {
  hash_combine(seed, static_cast<double>(value));
}

inline void hash_combine(std::size_t &seed, const std::string &value) {
  for (const auto &c : value) {
    hash_combine(seed, c);
  }
}
template <typename T>
inline void hash_combine(std::size_t &seed, const std::complex<T> &value) {
  hash_combine(seed, value.real());
  hash_combine(seed, value.imag());
}

template <typename T>
inline void hash_combine(std::size_t &seed, const std::vector<T> &value) {
  for (const auto &c : value) {
    hash_combine(seed, c);
  }
}

} // namespace numsim::cas

#endif // HASH_FUNCTIONS_H
