#ifndef TENSOR_TO_SCALAR_STD_H
#define TENSOR_TO_SCALAR_STD_H

#include "../expression_holder.h"
#include "../numsim_cas_type_traits.h"
#include "visitors/tensor_scalar_printer.h"
#include <sstream>

namespace std {

template <typename ValueType>
auto to_string(numsim::cas::expression_holder<
               numsim::cas::tensor_to_scalar_expression<ValueType>> &&expr) {
  std::stringstream ss;
  numsim::cas::tensor_to_scalar_printer<ValueType, std::stringstream> printer(
      ss);
  printer.apply(expr);
  return ss.str();
}

template <typename ValueType>
auto to_string(const numsim::cas::expression_holder<
               numsim::cas::tensor_to_scalar_expression<ValueType>> &expr) {
  std::stringstream ss;
  numsim::cas::tensor_to_scalar_printer<ValueType, std::stringstream> printer(
      ss);
  printer.apply(expr);
  return ss.str();
}

} // namespace std

#endif // TENSOR_TO_SCALAR_STD_H
