#ifndef KRONECKER_DELTA_H
#define KRONECKER_DELTA_H

#include "../numsim_cas_type_traits.h"

namespace numsim::cas {

template<typename ValueType>
class kronecker_delta final : public expression_crtp<kronecker_delta<ValueType>, tensor_expression<ValueType>> {
public:
  using base_expr = expression_crtp<kronecker_delta<ValueType>, tensor_expression<ValueType>>;
  using value_type = ValueType;
  kronecker_delta(std::size_t dim) : base_expr(dim, 2) {}
  kronecker_delta(kronecker_delta const& data) : base_expr(data.m_dim, 2) {}
};

} // NAMESPACE symTM

#endif // KRONECKER_DELTA_H
