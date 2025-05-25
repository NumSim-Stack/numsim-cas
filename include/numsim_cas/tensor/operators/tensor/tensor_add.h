#ifndef TENSOR_ADD_H
#define TENSOR_ADD_H

#include "../../../n_ary_tree.h"
#include "../../../numsim_cas_type_traits.h"

namespace numsim::cas {

template <typename ValueType>
class tensor_add final : public n_ary_tree<tensor_expression<ValueType>, tensor_add<ValueType>> {
public:
  using base = n_ary_tree<tensor_expression<ValueType>, tensor_add<ValueType>>;

  tensor_add(std::size_t dim, std::size_t rank):base(dim, rank){}
  tensor_add(tensor_add const& add):base(static_cast<const base&>(add), add.dim(), add.rank()){}
  tensor_add(tensor_add && add):base(static_cast<base&&>(add), add.dim(), add.rank()){}
  ~tensor_add() = default;
  const tensor_add &operator=(tensor_add &&) = delete;
};

} // NAMESPACE numsim::cas

#endif // TENSOR_ADD_H
