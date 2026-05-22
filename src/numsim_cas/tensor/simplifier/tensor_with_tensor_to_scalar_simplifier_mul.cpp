#include <numsim_cas/core/operators.h>
#include <numsim_cas/tensor/operators/scalar/tensor_scalar_mul.h>
#include <numsim_cas/tensor/simplifier/tensor_with_tensor_to_scalar_simplifier_mul.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

namespace numsim::cas {
namespace tensor_with_tensor_to_scalar_detail {
namespace simplifier {

// (T × f) × g  →  T × (f × g)
// Collapse nested tensor_to_scalar_with_tensor_mul on the lhs by
// pushing the inner-lhs tensor outward and folding the two t2s
// factors via the existing t2s × t2s mul (which has its own
// simplifier).
mul_base::expr_holder_tensor_t
mul_base::dispatch(tensor_to_scalar_with_tensor_mul const &lhs) noexcept {
  return lhs.expr_lhs() * (lhs.expr_rhs() * m_rhs);
}

// (s × T) × g  →  s × (T × g)
// Bubble the scalar coefficient outward. The inner T × g re-enters
// this simplifier (terminates at the default since T is no longer a
// tensor_scalar_mul nor a t2s-with-tensor-mul under the canonicalised
// build path). The outer s × (...) routes through the existing
// tensor × scalar operator.
//
// `tensor_scalar_mul` operand convention: `expr_lhs()` is the
// *scalar* coefficient and `expr_rhs()` is the *tensor* body. (Yes
// it's lhs=scalar even though the node is named "tensor_scalar_mul";
// match the convention used by tensor_with_scalar_simplifier_mul.cpp,
// which also computes `rhs.expr_lhs() * scalar` to merge
// coefficients.) Misreading this swap would silently corrupt the
// product — read either site to confirm before changing.
mul_base::expr_holder_tensor_t
mul_base::dispatch(tensor_scalar_mul const &lhs) noexcept {
  return lhs.expr_lhs() * (lhs.expr_rhs() * m_rhs);
}

} // namespace simplifier
} // namespace tensor_with_tensor_to_scalar_detail
} // namespace numsim::cas
