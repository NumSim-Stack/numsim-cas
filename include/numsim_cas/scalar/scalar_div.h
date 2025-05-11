#ifndef SCALAR_DIV_H
#define SCALAR_DIV_H

#include "../binary_op.h"

namespace numsim::cas {

template <typename ValueType>
class scalar_div final
    : public binary_op<scalar_div<ValueType>, scalar_expression<ValueType>> {
public:
  using base = binary_op<scalar_div<ValueType>, scalar_expression<ValueType>>;

  using base::base;
  scalar_div() = delete;
  scalar_div(scalar_div&&data):base(std::move(static_cast<base&&>(data))){}
  scalar_div(scalar_div const &data):base(static_cast<base const&>(data)){}
  ~scalar_div() = default;
  const scalar_div &operator=(scalar_div &&) = delete;
};

} // NAMESPACE symTM

#endif // SCALAR_DIV_H
