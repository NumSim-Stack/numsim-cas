#ifndef TENSOR_GLOBALS_H
#define TENSOR_GLOBALS_H

#include "../basic_functions.h"
#include "kronecker_delta.h"
// #include "tensor_zero.h"

namespace numsim::cas {
namespace detail {
auto tensor_one_1_d{make_expression<kronecker_delta<double>>(1)};
auto tensor_one_2_d{make_expression<kronecker_delta<double>>(2)};
auto tensor_one_3_d{make_expression<kronecker_delta<double>>(3)};

auto tensor_one_1_f{make_expression<kronecker_delta<double>>(1)};
auto tensor_one_2_f{make_expression<kronecker_delta<double>>(2)};
auto tensor_one_3_f{make_expression<kronecker_delta<double>>(3)};

auto tensor_one_1_i{make_expression<kronecker_delta<double>>(1)};
auto tensor_one_2_i{make_expression<kronecker_delta<double>>(2)};
auto tensor_one_3_i{make_expression<kronecker_delta<double>>(3)};
} // namespace detail

template <typename T>
constexpr inline const auto &get_identity_tensor(std::size_t dim);

template <>
constexpr inline const auto &get_identity_tensor<double>(std::size_t dim) {
  switch (dim) {
  case 1:
    return detail::tensor_one_1_d;
  case 2:
    return detail::tensor_one_2_d;
  case 3:
    return detail::tensor_one_3_d;
  default:
    assert(true);
    return detail::tensor_one_3_d;
  }
}

template <>
constexpr inline const auto &get_identity_tensor<float>(std::size_t dim) {
  switch (dim) {
  case 1:
    return detail::tensor_one_1_f;
  case 2:
    return detail::tensor_one_2_f;
  case 3:
    return detail::tensor_one_3_f;
  default:
    assert(true);
    return detail::tensor_one_3_f;
  }
}

template <>
constexpr inline const auto &get_identity_tensor<int>(std::size_t dim) {
  switch (dim) {
  case 1:
    return detail::tensor_one_1_i;
  case 2:
    return detail::tensor_one_2_i;
  case 3:
    return detail::tensor_one_3_i;
  default:
    assert(true);
    return detail::tensor_one_3_i;
  }
}

} // namespace numsim::cas

#endif // TENSOR_GLOBALS_H
