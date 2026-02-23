#include <numsim_cas/core/operators.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor/simplifier/tensor_with_scalar_simplifier_mul.h>

namespace numsim::cas {
namespace tensor_with_scalar_detail {
namespace simplifier {

// lhs := scalar_expression
// rhs := tensor_expression

mul_base::expr_holder_tensor_t
mul_base::dispatch(tensor_scalar_mul const &rhs) noexcept {
  return make_expression<tensor_scalar_mul>(rhs.expr_lhs() * m_lhs,
                                            rhs.expr_rhs());
}

} // namespace simplifier
} // namespace tensor_with_scalar_detail
} // namespace numsim::cas
