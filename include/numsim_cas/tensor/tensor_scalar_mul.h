#ifndef TENSOR_SCALAR_MUL_H
#define TENSOR_SCALAR_MUL_H

#include <stdexcept>
#include "../numsim_cas_type_traits.h"
#include "../binary_op.h"

namespace numsim::cas {

template<typename ValueType>
class tensor_scalar_mul final : public binary_op<tensor_scalar_mul<ValueType>, scalar_expression<ValueType>, tensor_expression<ValueType>>
{
public:
  using base = binary_op<tensor_scalar_mul<ValueType>, scalar_expression<ValueType>, tensor_expression<ValueType>>;

  template <typename LHS, typename RHS>
  tensor_scalar_mul(LHS &&lhs, RHS &&rhs)
      : base(std::forward<LHS>(lhs), std::forward<RHS>(rhs), rhs.get().dim(), rhs.get().rank()),
        m_constant(is_same<scalar_constant<ValueType>>(this->m_lhs))
  {
    update_hash();
  }

  template <typename LHS, typename RHS>
  tensor_scalar_mul(LHS const&lhs, RHS const&rhs)
      : base(lhs, rhs, rhs.get().dim(), rhs.get().rank()),
        m_constant(is_same<scalar_constant<ValueType>>(this->m_lhs))
  {
    update_hash();
  }

  inline void update_hash(){
    //static const auto id{base::get_id()};
    base::m_hash_value = 0;
    //hash_combine(base::m_hash_value, id);
     if(!m_constant){
      hash_combine(base::m_hash_value, this->m_lhs.get().hash_value());
    }
    hash_combine(base::m_hash_value, this->m_rhs.get().hash_value());
    base::m_hash_value= this->m_rhs.get().hash_value();
  }

private:
  bool m_constant;
};

} // NAMESPACE symTM

#endif // TENSOR_SCALAR_MUL_H
