#ifndef SCALAR_EXP_H
#define SCALAR_EXP_H

#include "../unary_op.h"

namespace numsim::cas {
template <typename ValueType>
class scalar_exp final
    : public unary_op<scalar_exp<ValueType>, scalar_expression<ValueType>> {
public:
  using base = unary_op<scalar_exp<ValueType>, scalar_expression<ValueType>>;

  using base::base;
  scalar_exp(scalar_exp const &expr) : base(static_cast<base const &>(expr)) {}
  scalar_exp(scalar_exp &&expr) : base(std::move(static_cast<base &&>(expr))) {}
  scalar_exp() = delete;
  ~scalar_exp() = default;
  const scalar_exp &operator=(scalar_exp &&) = delete;

private:
};
} // namespace numsim::cas

#endif // SCALAR_EXP_H
