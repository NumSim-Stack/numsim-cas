#ifndef SCALAR_FUNCTION_RULES_H
#define SCALAR_FUNCTION_RULES_H

#include <optional>

#include <numsim_cas/scalar/scalar_expression.h>

// Construction-time fold rules for the scalar unary functions (#417).
//
// Rule contract: each rule is a named, group-tagged free function
//     std::optional<holder> try_<name>(holder const& arg);
// returning nullopt when it does not fire. The function factories in
// scalar_std.h drive the applicable rules in sequence at construction
// (the "hand-sequence while small" form); the same functions are the
// entries a later registry would enumerate. Rules stay individually
// unit-testable, unlike the previous inline if-chains.
//
// Bodies live in scalar_function_rules.cpp so the operators/factories the
// rewrites call (operator-, pow, sqrt, ...) are visible via ADL there.

namespace numsim::cas::scalar_rules {

using holder = expression_holder<scalar_expression>;

// ── sin [trig] ───────────────────────────────────────────────────────
std::optional<holder> try_sin_zero(holder const &e);    // sin(0) → 0
std::optional<holder> try_sin_inverse(holder const &e); // sin(asin x) → x
std::optional<holder> try_sin_odd(holder const &e);     // sin(-x) → -sin(x)
std::optional<holder> try_sin_of_acos(holder const &e); // sin(acos x) → √(1-x²)
std::optional<holder>
try_sin_of_atan(holder const &e); // sin(atan x) → x/√(1+x²)

// ── cos [trig] ───────────────────────────────────────────────────────
std::optional<holder> try_cos_zero(holder const &e);    // cos(0) → 1
std::optional<holder> try_cos_inverse(holder const &e); // cos(acos x) → x
std::optional<holder> try_cos_even(holder const &e);    // cos(-x) → cos(x)
std::optional<holder> try_cos_of_asin(holder const &e); // cos(asin x) → √(1-x²)
std::optional<holder>
try_cos_of_atan(holder const &e); // cos(atan x) → 1/√(1+x²)

// ── tan [trig] ───────────────────────────────────────────────────────
std::optional<holder> try_tan_zero(holder const &e);    // tan(0) → 0
std::optional<holder> try_tan_inverse(holder const &e); // tan(atan x) → x
std::optional<holder> try_tan_odd(holder const &e);     // tan(-x) → -tan(x)
std::optional<holder>
try_tan_of_asin(holder const &e); // tan(asin x) → x/√(1-x²)

// ── asin/acos/atan [trig-inverse] ────────────────────────────────────
std::optional<holder> try_asin_zero(holder const &e); // asin(0) → 0
std::optional<holder> try_asin_odd(holder const &e);  // asin(-x) → -asin(x)
std::optional<holder> try_acos_one(holder const &e);  // acos(1) → 0
std::optional<holder> try_atan_zero(holder const &e); // atan(0) → 0
std::optional<holder> try_atan_odd(holder const &e);  // atan(-x) → -atan(x)

// ── exp/log [exp-log] ────────────────────────────────────────────────
std::optional<holder> try_exp_zero(holder const &e);   // exp(0) → 1
std::optional<holder> try_exp_of_log(holder const &e); // exp(log x) → x
std::optional<holder> try_log_one(holder const &e);    // log(1) → 0
std::optional<holder> try_log_of_exp(holder const &e); // log(exp x) → x
std::optional<holder>
try_log_of_sqrt(holder const &e); // log(sqrt x) → log(x)/2
std::optional<holder>
try_log_of_pow(holder const &e); // log(x^n) → n·log(x), x>0

// ── sqrt [sqrt] ──────────────────────────────────────────────────────
std::optional<holder> try_sqrt_zero(holder const &e);      // sqrt(0) → 0
std::optional<holder> try_sqrt_one(holder const &e);       // sqrt(1) → 1
std::optional<holder> try_sqrt_of_square(holder const &e); // sqrt(x^2) → x, x≥0
std::optional<holder>
try_sqrt_of_exp(holder const &e); // sqrt(exp x) → exp(x/2)

// ── abs/sign [sign-cone] ─────────────────────────────────────────────
std::optional<holder> try_abs_nonneg(holder const &e);    // |x| → x,  x≥0
std::optional<holder> try_abs_nonpos(holder const &e);    // |x| → -x, x≤0
std::optional<holder> try_sign_zero(holder const &e);     // sign(0) → 0
std::optional<holder> try_sign_positive(holder const &e); // sign(x) → 1,  x>0
std::optional<holder> try_sign_negative(holder const &e); // sign(x) → -1, x<0

} // namespace numsim::cas::scalar_rules

#endif // SCALAR_FUNCTION_RULES_H
