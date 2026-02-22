#ifndef SIMPLE_OUTER_PRODUCT_H
#define SIMPLE_OUTER_PRODUCT_H

#include <numsim_cas/core/n_ary_vector.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

class simple_outer_product final
    : public n_ary_vector<tensor_node_base_t<simple_outer_product>> {
public:
  using base = n_ary_vector<tensor_node_base_t<simple_outer_product>>;

  simple_outer_product(std::size_t dim, std::size_t rank) : base(dim, rank) {}
  simple_outer_product(simple_outer_product const &add)
      : base(static_cast<const base &>(add), add.dim(), add.rank()) {}
  simple_outer_product(simple_outer_product &&add) noexcept
      : base(static_cast<base &&>(add), add.dim(), add.rank()) {}
  ~simple_outer_product() override = default;
  const simple_outer_product &operator=(simple_outer_product &&) = delete;
};

} // NAMESPACE numsim::cas

#endif // SIMPLE_OUTER_PRODUCT_H
