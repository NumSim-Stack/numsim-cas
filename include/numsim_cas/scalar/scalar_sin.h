#ifndef SCALAR_SIN_H
#define SCALAR_SIN_H

#include "../scalar/scalar_expression.h"
#include "../unary_op.h"

namespace numsim::cas {

template <typename ValueType>
class scalar_sin final
    : public unary_op<scalar_sin<ValueType>, scalar_expression<ValueType>> {
public:
  using base = unary_op<scalar_sin<ValueType>, scalar_expression<ValueType>>;

  using base::base;
  scalar_sin(scalar_sin const &expr) : base(static_cast<base const &>(expr)) {}
  scalar_sin(scalar_sin &&expr) : base(std::move(static_cast<base &&>(expr))) {}
  scalar_sin() = delete;
  ~scalar_sin() = default;
  const scalar_sin &operator=(scalar_sin &&) = delete;

private:
};
} // namespace numsim::cas

#endif // SCALAR_SIN_H
