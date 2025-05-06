#ifndef UTILITY_FUNC_H
#define UTILITY_FUNC_H

#include <tuple>
#include <string>
#include "symTM_type_traits.h"
#include "operators.h"


//#include "expression.h"

namespace symTM {




// Helper function for combining hashes
template <typename T>
inline void hash_combine(std::size_t& seed, const T& value) {
  //std::hash<T> hasher;
  seed ^= static_cast<std::size_t>(value) + static_cast<std::size_t>(0x9e3779b9) + (seed << 6) + (seed >> 2);
}

inline void hash_combine(std::size_t& seed, const std::string& value) {
  for(const auto& c : value){
    hash_combine(seed, c);
  }
}



template<typename ...Args>
constexpr inline auto tuple(Args &&...args){
  return std::make_tuple(std::forward<Args>(args)...);
}





}

#endif // UTILITY_FUNC_H
