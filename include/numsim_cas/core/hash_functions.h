#ifndef HASH_FUNCTIONS_H
#define HASH_FUNCTIONS_H

#include <complex>
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
