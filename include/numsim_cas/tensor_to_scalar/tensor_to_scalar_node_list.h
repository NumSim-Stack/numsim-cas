#ifndef TENSOR_TO_SCALAR_NODE_LIST_H
#define TENSOR_TO_SCALAR_NODE_LIST_H

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
  NEXT(tensor_to_scalar_exp)                                                   \
  NEXT(tensor_to_scalar_sqrt)                                                  \
  NEXT(tensor_to_scalar_scalar_wrapper)

#endif // TENSOR_TO_SCALAR_NODE_LIST_H
