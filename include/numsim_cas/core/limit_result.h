#ifndef LIMIT_RESULT_H
#define LIMIT_RESULT_H

namespace numsim::cas {

struct growth_rate {
  enum class type { constant, logarithmic, polynomial, exponential, unknown };
  type rate = type::constant;
  double exponent = 0; // poly: degree, log: power of log, exp: base
};

struct limit_result {
  enum class direction {
    zero,            // -> 0
    finite_positive, // -> c > 0
    finite_negative, // -> c < 0
    pos_infinity,    // -> +inf
    neg_infinity,    // -> -inf
    indeterminate,   // 0/0, inf-inf, 0*inf
    unknown          // can't determine
  };
  direction dir = direction::unknown;
  growth_rate rate{};
};

struct limit_target {
  enum class point { zero_plus, zero_minus, pos_infinity, neg_infinity };
  point target;
};

} // namespace numsim::cas

#endif // LIMIT_RESULT_H
