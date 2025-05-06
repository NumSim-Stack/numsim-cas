#ifndef SCALAR_SIGN_H
#define SCALAR_SIGN_H

#include "../unary_op.h"

namespace numsim::cas {
template <typename ValueType>
class scalar_sign final : public unary_op<scalar_sign<ValueType>, scalar_expression<ValueType>>
{
public:
  using base = unary_op<scalar_sign<ValueType>, scalar_expression<ValueType>>;

  using base::base;
  scalar_sign(scalar_sign const& expr):base(static_cast<base const&>(expr)) {}
  scalar_sign(scalar_sign && expr):base(std::move(static_cast<base &&>(expr))) {}
  scalar_sign() = delete;
  ~scalar_sign() = default;
  const scalar_sign &operator=(scalar_sign &&) = delete;
private:
};
}

#endif // SCALAR_SIGN_H
