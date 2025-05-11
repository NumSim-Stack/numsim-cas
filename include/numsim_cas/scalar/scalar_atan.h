#ifndef SCALAR_ATAN_H
#define SCALAR_ATAN_H

#include "../unary_op.h"
namespace numsim::cas {
template <typename ValueType>
class scalar_atan final : public unary_op<scalar_atan<ValueType>, scalar_expression<ValueType>>
{
public:
  using base = unary_op<scalar_atan<ValueType>, scalar_expression<ValueType>>;

  using base::base;
  scalar_atan(scalar_atan const& expr):base(static_cast<base const&>(expr)) {}
  scalar_atan(scalar_atan && expr):base(std::move(static_cast<base &&>(expr))) {}
  scalar_atan() = delete;
  ~scalar_atan() = default;
  const scalar_atan &operator=(scalar_atan &&) = delete;
private:
};
}


#endif // SCALAR_ATAN_H
