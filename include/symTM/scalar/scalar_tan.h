#ifndef SCALAR_TAN_H
#define SCALAR_TAN_H

#include "../unary_op.h"

namespace symTM {
template <typename ValueType>
class scalar_tan final : public unary_op<scalar_tan<ValueType>, scalar_expression<ValueType>>
{
public:
  using base = unary_op<scalar_tan<ValueType>, scalar_expression<ValueType>>;
  using base::base;
  scalar_tan(scalar_tan const& expr):base(static_cast<base const&>(expr)) {}
  scalar_tan(scalar_tan && expr):base(std::move(static_cast<base &&>(expr))) {}
  scalar_tan() = delete;
  ~scalar_tan() = default;
  const scalar_tan &operator=(scalar_tan &&) = delete;
private:
};
}

#endif // SCALAR_TAN_H
