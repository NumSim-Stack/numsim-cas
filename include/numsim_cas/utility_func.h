#ifndef UTILITY_FUNC_H
#define UTILITY_FUNC_H

#include "numsim_cas_forward.h"
#include "numsim_cas_type_traits.h"
#include <string>
#include <tuple>

// #include "expression.h"

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

template <typename... Args> constexpr inline auto tuple(Args &&...args) {
  return std::make_tuple(std::forward<Args>(args)...);
}

template <typename BaseExpr>
constexpr inline const auto &
get(expression_holder<BaseExpr> const &expr_holder) {
  return std::visit([](const auto &expr) { return std::ref(expr); },
                    *expr_holder)
      .get();
}

} // namespace numsim::cas

#endif // UTILITY_FUNC_H
