#ifndef TENSOR_SUB_H
#define TENSOR_SUB_H

#include "../n_ary_tree.h"
#include "../numsim_cas_type_traits.h"

namespace numsim::cas {

//class tensor_sub final : public n_ary_tree, public VisitableTensorImpl_t<tensor_sub> {
//public:
//  using basis = VisitableTensorImpl_t<tensor_sub>;

//  tensor_sub(std::size_t dim, std::size_t rank) : n_ary_tree(),
//                                                  basis(dim, rank)
//  {}

//  tensor_sub(tensor_sub const &) = delete;
//  tensor_sub(tensor_sub&&) = delete;
//  ~tensor_sub() = default;
//  const tensor_sub& operator=(tensor_sub const &) = delete;
//};

} // NAMESPACE symTM

#endif // TENSOR_SUB_H
