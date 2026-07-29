#ifndef TENSOR_TO_SCALAR_FUNCTION_RULES_H
#define TENSOR_TO_SCALAR_FUNCTION_RULES_H

#include <optional>

#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

// Construction-time fold rules for the t2s functions (#417 / #420).
//
// Rule contract: each rule is a named, group-tagged free function
//     std::optional<t2s_holder> try_<name>(tensor_holder const& arg);
// returning nullopt when it does not fire. The trace/det/norm/dot functions in
// tensor_to_scalar_functions.cpp drive the applicable rules in sequence at
// construction. Bodies live in the matching .cpp because they call the t2s /
// tensor / scalar operators (operator*, /, pow, abs), which must be visible via
// ADL there — the same .h-decl / .cpp-def split as scalar_function_rules.
//
// These rules are cross-domain: they take a tensor and yield a t2s scalar. The
// catalog is the enumerable list a later registry would consume.
namespace numsim::cas::t2s_rules {

using tensor_holder = expression_holder<tensor_expression>;
using t2s_holder = expression_holder<tensor_to_scalar_expression>;

// ── dot / dot_product [dot] ──────────────────────────────────────────
std::optional<t2s_holder> try_dot_product_zero(tensor_holder const &lhs,
                                               tensor_holder const &rhs);
std::optional<t2s_holder> try_dot_zero(tensor_holder const &e); // dot(0) → 0

// ── trace [trace] ────────────────────────────────────────────────────
std::optional<t2s_holder> try_trace_zero(tensor_holder const &e); // tr(0) → 0
std::optional<t2s_holder>
try_trace_identity(tensor_holder const &e); // tr(I) → dim
std::optional<t2s_holder>
try_trace_of_trans(tensor_holder const &e); // tr(Aᵀ) → tr(A)
std::optional<t2s_holder>
try_trace_scalar_mul(tensor_holder const &e); // tr(s·A) → s·tr(A)
std::optional<t2s_holder>
try_trace_add(tensor_holder const &e); // tr(A+B) → tr(A)+tr(B)

// ── norm [norm] ──────────────────────────────────────────────────────
std::optional<t2s_holder> try_norm_zero(tensor_holder const &e); // ‖0‖ → 0
std::optional<t2s_holder>
try_norm_of_trans(tensor_holder const &e); // ‖Aᵀ‖ → ‖A‖
std::optional<t2s_holder>
try_norm_scalar_mul(tensor_holder const &e); // ‖s·A‖ → |s|·‖A‖

// ── det [det] ────────────────────────────────────────────────────────
std::optional<t2s_holder> try_det_zero(tensor_holder const &e); // det(0) → 0
std::optional<t2s_holder>
try_det_identity(tensor_holder const &e); // det(I) → 1
std::optional<t2s_holder>
try_det_chirality(tensor_holder const &e); // proper→1, improper→-1
std::optional<t2s_holder>
try_det_inv(tensor_holder const &e); // det(A⁻¹) → 1/det(A)
std::optional<t2s_holder>
try_det_trans(tensor_holder const &e); // det(Aᵀ) → det(A)
std::optional<t2s_holder>
try_det_outer_product(tensor_holder const &e); // det(u⊗v) → 0, dim≥2
std::optional<t2s_holder>
try_det_scalar_mul(tensor_holder const &e); // det(s·A) → sᵈ·det(A)
std::optional<t2s_holder>
try_det_mul(tensor_holder const &e); // det(∏Aᵢ) → ∏det(Aᵢ)

// ── exp / sqrt [math] ────────────────────────────────────────────────
// (t2s log has no construction folds.) Argument is a t2s scalar, not a tensor.
std::optional<t2s_holder> try_exp_zero(t2s_holder const &e);   // exp(0) → 1
std::optional<t2s_holder> try_exp_of_log(t2s_holder const &e); // exp(log x) → x
std::optional<t2s_holder> try_sqrt_zero(t2s_holder const &e); // sqrt(0) → 0
std::optional<t2s_holder> try_sqrt_one(t2s_holder const &e);  // sqrt(1) → 1
std::optional<t2s_holder>
try_sqrt_wrapper_one(t2s_holder const &e); // sqrt(⟨1⟩) → 1

} // namespace numsim::cas::t2s_rules

#endif // TENSOR_TO_SCALAR_FUNCTION_RULES_H
