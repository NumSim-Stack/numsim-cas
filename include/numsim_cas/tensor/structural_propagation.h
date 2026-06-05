#ifndef NUMSIM_CAS_TENSOR_STRUCTURAL_PROPAGATION_H
#define NUMSIM_CAS_TENSOR_STRUCTURAL_PROPAGATION_H

#include <numsim_cas/tensor/tensor_expression.h>

// Centralized structural-fact propagation rules for compound tensor
// wrappers. Each compound wrapper that derives a structural classification
// from its child(ren) calls one of these helpers from its constructor.
//
// Replaces the previous pattern where each wrapper inlined a 2-line
// `if (auto const& sp = child.space()) set_space(*sp);` in its ctor —
// scattered across 16 sites. SymPy-redesign step 3a: extract the
// trivial pass-through case; subsequent commits migrate the more
// substantial per-wrapper rules (tensor_pow's closure rules,
// tensor_inv's PD/PSD inheritance, tensor_add's n-ary join).
//
// Each helper writes into the SAME storage today (m_tensor_space /
// m_tensor_algebra_assumptions) — no new field. The visitor pattern was
// considered and rejected because no consumer needs dynamic dispatch:
// each compound wrapper knows its own type at construction.

namespace numsim::cas::structural_propagation {

// Unary preserve: the result inherits the child's space classification
// unchanged. Used by tensor_negative (−A has the same Sym/Skew/Vol/Dev
// as A) and tensor_scalar_mul (α·A has the same space as A for any α).
//
// No-op if the child has no space classification.
inline void preserve_unary(tensor_expression *out,
                           tensor_expression const &child) noexcept {
  if (auto const &sp = child.space())
    out->set_space(*sp);
}

} // namespace numsim::cas::structural_propagation

#endif // NUMSIM_CAS_TENSOR_STRUCTURAL_PROPAGATION_H
