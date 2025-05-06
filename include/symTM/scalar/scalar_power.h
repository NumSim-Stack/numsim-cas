#ifndef SCALAR_POWER_H
#define SCALAR_POWER_H

#include "../binary_op.h"

namespace numsim::cas {
template <typename ValueType>
class scalar_pow final : public binary_op<scalar_pow<ValueType>, scalar_expression<ValueType>>
{
public:
  using base = binary_op<scalar_pow<ValueType>, scalar_expression<ValueType>>;

  using base::base;
  scalar_pow(scalar_pow const& expr):base(static_cast<base const&>(expr)) {}
  scalar_pow(scalar_pow && expr):base(std::move(static_cast<base &&>(expr))) {}
  scalar_pow() = delete;
  ~scalar_pow() = default;
  const scalar_pow &operator=(scalar_pow &&) = delete;

  inline void update_hash(){
//    base::m_hash_value = 0;
//    if(!is_same<scalar_constant<ValueType>>(this->m_rhs)){
//      hash_combine(base::m_hash_value, this->m_lhs.get().hash_value());
//      hash_combine(base::m_hash_value, this->m_rhs.get().hash_value());
//    }else{
//      base::m_hash_value = this->m_lhs.get().hash_value();
//    }
    base::m_hash_value = this->m_lhs.get().hash_value();
  }
private:
};
}

#endif // SCALAR_POWER_H
