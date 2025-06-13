#ifndef SCALAR_ONE_H
#define SCALAR_ONE_H

#include "../utility_func.h"
#include "scalar_expression.h"

namespace numsim::cas {

template <typename ValueType>
class scalar_one final : public expression_crtp<scalar_one<ValueType>,
                                                scalar_expression<ValueType>> {
public:
  using base =
      expression_crtp<scalar_one<ValueType>, scalar_expression<ValueType>>;
  scalar_one() { hash_combine(this->m_hash_value, base::get_id()); }
  scalar_one(scalar_one &&data) : base(std::move(static_cast<base &&>(data))) {}
  scalar_one(scalar_one const &data) : base(static_cast<base const &>(data)) {}
  ~scalar_one() = default;
  const scalar_one &operator=(scalar_one &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_ONE_H
