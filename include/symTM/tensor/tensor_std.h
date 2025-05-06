#ifndef TENSOR_STD_H
#define TENSOR_STD_H

#include "../expression_holder.h"
#include "../symTM_type_traits.h"
#include "tensor_printer.h"
#include <sstream>

namespace std {

template <typename ValueType>
auto to_string(
    symTM::expression_holder<symTM::tensor_expression<ValueType>> &&expr) {
  std::stringstream ss;
  symTM::tensor_printer<ValueType, std::stringstream> printer(ss);
  printer.apply(expr);
  return ss.str();
}

template <typename ValueType>
auto to_string(
    symTM::expression_holder<symTM::tensor_expression<ValueType>> &expr) {
  std::stringstream ss;
  symTM::tensor_printer<ValueType, std::stringstream> printer(ss);
  printer.apply(expr);
  return ss.str();
}

} // NAMESPACE STD

#endif // TENSOR_STD_H
