#ifndef SCALAR_NEGATIV_H
#define SCALAR_NEGATIV_H

#include "../unary_op.h"

namespace numsim::cas {

template <typename ValueType>
class scalar_negative final : public unary_op<scalar_negative<ValueType>,
                                              scalar_expression<ValueType>> {
public:
  using base =
      unary_op<scalar_negative<ValueType>, scalar_expression<ValueType>>;

  using base::base;
  scalar_negative(scalar_negative &&data)
      : base(std::move(static_cast<base &&>(data))) {}
  scalar_negative(scalar_negative const &data)
      : base(static_cast<base const &>(data)) {}
  scalar_negative() = delete;
  ~scalar_negative() = default;
  const scalar_negative &operator=(scalar_negative &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_NEGATIV_H
