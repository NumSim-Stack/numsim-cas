#ifndef SCALAR_NODE_LIST_H
#define SCALAR_NODE_LIST_H

// Usage: NUMSIM_CAS_SCALAR_NODE_LIST(FIRST_MACRO, NEXT_MACRO)
// FIRST_MACRO(T) expands the first element (no leading comma)
// NEXT_MACRO(T) expands subsequent elements (with leading comma)

// NEXT(scalar_div)

#define NUMSIM_CAS_SCALAR_NODE_LIST(FIRST, NEXT)                               \
  FIRST(scalar)                                                                \
  NEXT(scalar_zero)                                                            \
  NEXT(scalar_one)                                                             \
  NEXT(scalar_constant)                                                        \
  NEXT(scalar_add)                                                             \
  NEXT(scalar_mul)                                                             \
  NEXT(scalar_negative)                                                        \
  NEXT(scalar_function)                                                        \
  NEXT(scalar_sin)                                                             \
  NEXT(scalar_cos)                                                             \
  NEXT(scalar_tan)                                                             \
  NEXT(scalar_asin)                                                            \
  NEXT(scalar_acos)                                                            \
  NEXT(scalar_atan)                                                            \
  NEXT(scalar_pow)                                                             \
  NEXT(scalar_sqrt)                                                            \
  NEXT(scalar_log)                                                             \
  NEXT(scalar_exp)                                                             \
  NEXT(scalar_sign)                                                            \
  NEXT(scalar_abs)                                                             \
  NEXT(scalar_rational)

#endif // SCALAR_NODE_LIST_H
