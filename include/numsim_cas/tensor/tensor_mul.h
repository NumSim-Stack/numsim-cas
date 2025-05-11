#ifndef TENSOR_MUL_H
#define TENSOR_MUL_H

#include "../n_ary_tree.h"
#include "../numsim_cas_type_traits.h"

namespace numsim::cas {

template <typename ValueType>
class tensor_mul final : public n_ary_tree<tensor_expression<ValueType>, tensor_add<ValueType>> {
public:
  using base = n_ary_tree<tensor_expression<ValueType>, tensor_add<ValueType>>;
  using base::base;

  tensor_mul(std::size_t dim, std::size_t rank):base(dim, rank){}
  tensor_mul(tensor_mul const& add):base(static_cast<const base&>(add), add.dim(), add.rank()){}
  tensor_mul(tensor_mul && add):base(static_cast<base&&>(add), add.dim(), add.rank()){}
  ~tensor_mul() = default;
  const tensor_mul &operator=(tensor_mul &&) = delete;
};

} // NAMESPACE numsim::cas

#endif // TENSOR_MUL_H
