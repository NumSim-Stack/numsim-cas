#ifndef UTILITY_FUNC_H
#define UTILITY_FUNC_H

#include <tuple>

namespace numsim::cas {
template <typename... Args> constexpr inline auto tuple(Args &&...args) {
  return std::make_tuple(std::forward<Args>(args)...);
}

} // namespace numsim::cas

#endif // UTILITY_FUNC_H
