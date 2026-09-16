#ifndef LIMIT_ALGEBRA_H
#define LIMIT_ALGEBRA_H

#include <numsim_cas/core/limit_result.h>

namespace numsim::cas {

class limit_algebra {
protected:
  static limit_result combine_add(limit_result a, limit_result b);
  static limit_result combine_mul(limit_result a, limit_result b);
  static limit_result apply_neg(limit_result a);
  // zero_from_above: the argument that tends to zero is known to stay
  // positive; without it 1/0, 0^(-c) and log(0) have no provable sign.
  static limit_result apply_log(limit_result a, bool zero_from_above);
  static limit_result apply_pow(limit_result base, limit_result exponent,
                                bool zero_from_above);
  static limit_result apply_sqrt(limit_result a, bool zero_from_above);
  static limit_result apply_abs(limit_result a);
  static limit_result apply_reciprocal(limit_result a, bool zero_from_above);
  static limit_result apply_exp(limit_result a);
};

} // namespace numsim::cas

#endif // LIMIT_ALGEBRA_H
