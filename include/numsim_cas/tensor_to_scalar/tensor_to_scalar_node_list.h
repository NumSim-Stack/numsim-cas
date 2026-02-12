#ifndef TENSOR_TO_SCALAR_NODE_LIST_H
#define TENSOR_TO_SCALAR_NODE_LIST_H

// class tensor_to_scalar_expression;
// class tensor_to_scalar_div;
// class tensor_to_scalar_with_scalar_div;
// class scalar_with_tensor_to_scalar_div;

#define NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(FIRST, NEXT)                     \
  FIRST(tensor_trace)                                                          \
  NEXT(tensor_dot)                                                             \
  NEXT(tensor_det)                                                             \
  NEXT(tensor_norm)                                                            \
  NEXT(tensor_to_scalar_negative)                                              \
  NEXT(tensor_to_scalar_add)                                                   \
  NEXT(tensor_to_scalar_mul)                                                   \
  NEXT(tensor_to_scalar_pow)                                                   \
  NEXT(tensor_inner_product_to_scalar)                                         \
  NEXT(tensor_to_scalar_zero)                                                  \
  NEXT(tensor_to_scalar_one)                                                   \
  NEXT(tensor_to_scalar_log)                                                   \
  NEXT(tensor_to_scalar_scalar_wrapper)
//  NEXT(tensor_to_scalar_pow_with_scalar_exponent)                              \
//  NEXT(tensor_to_scalar_with_scalar_add)
//  NEXT(tensor_to_scalar_with_scalar_mul)

#endif // TENSOR_TO_SCALAR_NODE_LIST_H
