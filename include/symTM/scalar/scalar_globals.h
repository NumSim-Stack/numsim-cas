#ifndef SCALAR_GLOBALS_H
#define SCALAR_GLOBALS_H

#include "../basic_functions.h"
#include "scalar_zero.h"
#include "scalar_one.h"

namespace numsim::cas {
namespace detail {
auto scalar_zero_d{make_expression<scalar_zero<double>>()};
auto scalar_zero_i{make_expression<scalar_zero<int>>()};
auto scalar_zero_f{make_expression<scalar_zero<float>>()};

auto scalar_one_d{make_expression<scalar_one<double>>()};
auto scalar_one_i{make_expression<scalar_one<int>>()};
auto scalar_one_f{make_expression<scalar_one<float>>()};
}
template<typename T>
constexpr inline auto& get_scalar_zero();
template<>
constexpr inline auto& get_scalar_zero<double>(){
  return detail::scalar_zero_d;
}
template<>
constexpr inline auto& get_scalar_zero<float>(){
  return detail::scalar_zero_f;
}
template<>
constexpr inline auto& get_scalar_zero<int>(){
  return detail::scalar_zero_i;
}

template<typename T>
constexpr inline auto& get_scalar_one();
template<>
constexpr inline auto& get_scalar_one<double>(){
  return detail::scalar_one_d;
}
template<>
constexpr inline auto& get_scalar_one<float>(){
  return detail::scalar_one_f;
}
template<>
constexpr inline auto& get_scalar_one<int>(){
  return detail::scalar_one_i;
}
}

#endif // SCALAR_GLOBALS_H
