#ifndef SCALAR_SQRT_H
#define SCALAR_SQRT_H

#include "../unary_op.h"
namespace numsim::cas {
template <typename ValueType>
class scalar_sqrt final : public unary_op<scalar_sqrt<ValueType>, scalar_expression<ValueType>>
{
public:
  using base = unary_op<scalar_sqrt<ValueType>, scalar_expression<ValueType>>;

  using base::base;
  scalar_sqrt(scalar_sqrt const& expr):base(static_cast<base const&>(expr)) {}
  scalar_sqrt(scalar_sqrt && expr):base(std::move(static_cast<base &&>(expr))) {}
  scalar_sqrt() = delete;
  ~scalar_sqrt() = default;
  const scalar_sqrt &operator=(scalar_sqrt &&) = delete;
private:
};
}

#endif // SCALAR_SQRT_H
