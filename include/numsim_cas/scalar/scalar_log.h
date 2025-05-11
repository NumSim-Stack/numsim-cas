#ifndef SCALAR_LOG_H
#define SCALAR_LOG_H

#include "../unary_op.h"

namespace numsim::cas {
template <typename ValueType>
class scalar_log final : public unary_op<scalar_expression<ValueType>, scalar_expression<ValueType>>
{
public:
  using base = unary_op<scalar_expression<ValueType>, scalar_expression<ValueType>>;

  using base::base;
  scalar_log(scalar_log const& expr):base(static_cast<base const&>(expr)) {}
  scalar_log(scalar_log && expr):base(std::move(static_cast<base &&>(expr))) {}
  scalar_log() = delete;
  ~scalar_log() = default;
  const scalar_log &operator=(scalar_log &&) = delete;
private:
};
}

#endif // SCALAR_LOG_H
