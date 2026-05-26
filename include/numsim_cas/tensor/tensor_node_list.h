#ifndef TENSOR_NODE_LIST_H
#define TENSOR_NODE_LIST_H

// tensor
// tensor_fundamentals := {symbol, 0, 1, constant}
// tensor_basic_operators := {+,-,*,/,negative}
// multiplication --> inner product of most right and most left index
// only devision by scalar
// tensor_product_functions := {inner, outer, simple_outer}

#define NUMSIM_CAS_TENSOR_NODE_LIST(FIRST, NEXT)                               \
  FIRST(tensor)                                                                \
  NEXT(tensor_add)                                                             \
  NEXT(tensor_mul)                                                             \
  NEXT(tensor_pow)                                                             \
  NEXT(tensor_negative)                                                        \
  NEXT(inner_product_wrapper)                                                  \
  NEXT(permute_indices_wrapper)                                                \
  NEXT(outer_product_wrapper)                                                  \
  NEXT(simple_outer_product)                                                   \
  NEXT(tensor_inv)                                                             \
  NEXT(tensor_zero)                                                            \
  NEXT(tensor_projector)                                                       \
  NEXT(identity_tensor)                                                        \
  NEXT(levi_civita_tensor)                                                     \
  NEXT(tensor_scalar_mul)                                                      \
  NEXT(tensor_to_scalar_with_tensor_mul)

#endif // TENSOR_NODE_LIST_H
