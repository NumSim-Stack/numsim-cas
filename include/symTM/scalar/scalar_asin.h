#ifndef SCALAR_ASIN_H
#define SCALAR_ASIN_H

#include "../unary_op.h"
namespace symTM {
template <typename ValueType>
class scalar_asin final : public unary_op<scalar_asin<ValueType>, scalar_expression<ValueType>>
{
public:
  using base = unary_op<scalar_asin<ValueType>, scalar_expression<ValueType>>;

  using base::base;
  scalar_asin(scalar_asin const& expr):base(static_cast<base const&>(expr)) {}
  scalar_asin(scalar_asin && expr):base(std::move(static_cast<base &&>(expr))) {}
  scalar_asin() = delete;
  ~scalar_asin() = default;
  const scalar_asin &operator=(scalar_asin &&) = delete;
private:
};
}

#endif // SCALAR_ASIN_H
