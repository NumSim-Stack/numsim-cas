#ifndef TENSOR_ZERO_H
#define TENSOR_ZERO_H

#include "../expression_crtp.h"
#include "tensor_expression.h"

namespace symTM {
template<typename ValueType>
class tensor_zero final : public expression_crtp<tensor_zero<ValueType>, tensor_expression<ValueType>> {
public:
  using base = expression_crtp<tensor_zero<ValueType>, tensor_expression<ValueType>>;

  tensor_zero(std::size_t dim, std::size_t rank):base(dim, rank){}
  tensor_zero(tensor_zero&&data):base(static_cast<base&&>(data), data.dim(), data.rank()){}
  tensor_zero(tensor_zero const &data):base(static_cast<base const&>(data), data.dim(), data.rank()){}
  ~tensor_zero() = default;
  const tensor_zero &operator=(tensor_zero &&) = delete;
};
} // NAMESPACE symTM

#endif // TENSOR_ZERO_H
