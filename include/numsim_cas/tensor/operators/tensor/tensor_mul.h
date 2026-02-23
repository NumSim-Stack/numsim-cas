#ifndef TENSOR_MUL_H
#define TENSOR_MUL_H

#include <numsim_cas/core/n_ary_vector.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

class tensor_mul final : public n_ary_vector<tensor_node_base_t<tensor_mul>> {
public:
  using base = n_ary_vector<tensor_node_base_t<tensor_mul>>;
  tensor_mul(std::size_t dim, std::size_t rank) : base(dim, rank) {}
  tensor_mul(tensor_mul const &add)
      : base(static_cast<const base &>(add), add.dim(), add.rank()) {}
  tensor_mul(tensor_mul &&add) noexcept
      : base(static_cast<base &&>(add), add.dim(), add.rank()) {}
  ~tensor_mul() override = default;
  const tensor_mul &operator=(tensor_mul &&) = delete;
};

} // NAMESPACE numsim::cas

#endif // TENSOR_MUL_H
