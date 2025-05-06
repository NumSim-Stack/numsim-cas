#ifndef SCALAR_COS_H
#define SCALAR_COS_H

#include "../unary_op.h"

namespace symTM {
template <typename ValueType>
class scalar_cos final : public unary_op<scalar_cos<ValueType>, scalar_expression<ValueType>>
{
public:
  using base = unary_op<scalar_cos<ValueType>, scalar_expression<ValueType>>;

  using base::base;
  scalar_cos(scalar_cos const& expr):base(static_cast<base const&>(expr)) {}
  scalar_cos(scalar_cos && expr):base(std::move(static_cast<base &&>(expr))) {}
  scalar_cos() = delete;
  ~scalar_cos() = default;
  const scalar_cos &operator=(scalar_cos &&) = delete;
private:
};
}
#endif // SCALAR_COS_H
