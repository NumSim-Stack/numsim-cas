#ifndef TENSOR_TYPE_DEFS_H
#define TENSOR_TYPE_DEFS_H

#include "visitor_base.h"

namespace numsim::cas {

class tensor_expression;
class tensor;
class tensor_add;
class tensor_mul;
class tensor_pow;
class tensor_power_diff;
class tensor_negative;
class inner_product_wrapper;
class basis_change_imp;
class outer_product_wrapper;
class kronecker_delta;
class simple_outer_product;
class tensor_symmetry;
class tensor_deviatoric;
class tensor_volumetric;
class tensor_inv;
class tensor_zero;
class identity_tensor;
class tensor_projector;
class tensor_scalar_mul;
class tensor_scalar_div;
class tensor_to_scalar_with_tensor_mul;
class tensor_to_scalar_with_tensor_div;
// det, adj, skew, vol, dev,

using tensor_visitor_t =
    visitor<tensor, tensor_negative, inner_product_wrapper, basis_change_imp,
            outer_product_wrapper, kronecker_delta, simple_outer_product,
            tensor_add, tensor_mul, tensor_symmetry, tensor_inv,
            tensor_deviatoric, tensor_volumetric, tensor_zero, tensor_pow,
            identity_tensor, tensor_projector, tensor_scalar_mul,
            tensor_power_diff, tensor_scalar_div,
            tensor_to_scalar_with_tensor_mul, tensor_to_scalar_with_tensor_div>;

using tensor_visitor_const_t = visitor_const<
    tensor, tensor_negative, inner_product_wrapper, basis_change_imp,
    outer_product_wrapper, kronecker_delta, simple_outer_product, tensor_add,
    tensor_mul, tensor_symmetry, tensor_inv, tensor_deviatoric,
    tensor_volumetric, tensor_zero, tensor_pow, identity_tensor,
    tensor_projector, tensor_scalar_mul, tensor_power_diff, tensor_scalar_div,
    tensor_to_scalar_with_tensor_mul, tensor_to_scalar_with_tensor_div>;

using tensor_visitable_t = visitable<
    tensor, tensor_negative, inner_product_wrapper, basis_change_imp,
    outer_product_wrapper, kronecker_delta, simple_outer_product, tensor_add,
    tensor_mul, tensor_symmetry, tensor_inv, tensor_deviatoric,
    tensor_volumetric, tensor_zero, tensor_pow, identity_tensor,
    tensor_projector, tensor_scalar_mul, tensor_power_diff, tensor_scalar_div,
    tensor_to_scalar_with_tensor_mul, tensor_to_scalar_with_tensor_div>;

template <typename T>
using tensor_node_base_t = visitable_impl<
    scalar_expression, T, tensor, tensor_negative, inner_product_wrapper,
    basis_change_imp, outer_product_wrapper, kronecker_delta,
    simple_outer_product, tensor_add, tensor_mul, tensor_symmetry, tensor_inv,
    tensor_deviatoric, tensor_volumetric, tensor_zero, tensor_pow,
    identity_tensor, tensor_projector, tensor_scalar_mul, tensor_power_diff,
    tensor_scalar_div, tensor_to_scalar_with_tensor_mul,
    tensor_to_scalar_with_tensor_div>;

} // namespace numsim::cas
#endif // TENSOR_TYPE_DEFS_H
