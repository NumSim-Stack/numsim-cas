#ifndef SCALAR_ABS_H
#define SCALAR_ABS_H

#include "../unary_op.h"
namespace numsim::cas {
template <typename ValueType>
class scalar_abs final
    : public unary_op<scalar_abs<ValueType>, scalar_expression<ValueType>> {
public:
  using base = unary_op<scalar_abs<ValueType>, scalar_expression<ValueType>>;

  using base::base;
  scalar_abs(scalar_abs const &expr) : base(static_cast<base const &>(expr)) {}
  scalar_abs(scalar_abs &&expr) : base(std::move(static_cast<base &&>(expr))) {}
  scalar_abs() = delete;
  ~scalar_abs() = default;
  const scalar_abs &operator=(scalar_abs &&) = delete;

private:
};

} // namespace numsim::cas

#endif // SCALAR_ABS_H
