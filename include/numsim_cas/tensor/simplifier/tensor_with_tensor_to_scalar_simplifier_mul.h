#ifndef TENSOR_WITH_TENSOR_TO_SCALAR_SIMPLIFIER_MUL_H
#define TENSOR_WITH_TENSOR_TO_SCALAR_SIMPLIFIER_MUL_H

#include <numsim_cas/tensor/operators/tensor_to_scalar/tensor_to_scalar_with_tensor_mul.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_zero.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_scalar_mul;

namespace tensor_with_tensor_to_scalar_detail {
namespace simplifier {

// lhs := tensor_expression
// rhs := tensor_to_scalar_expression
//
// Dispatches on the tensor side. The two specialised dispatches
// (defined in the .cpp because they construct nodes that need the
// tensor_to_scalar operators in scope) handle:
//   * tensor_scalar_mul lhs ⇒ bubble the scalar coefficient outward.
//   * tensor_to_scalar_with_tensor_mul lhs ⇒ collapse nested t2s muls.
// The default returns a freshly constructed tensor_to_scalar_with_tensor_mul.
class mul_base final : public tensor_visitor_return_expr_t {
public:
  using expr_holder_tensor_t = expression_holder<tensor_expression>;
  using expr_holder_t2s_t = expression_holder<tensor_to_scalar_expression>;

  mul_base(expr_holder_tensor_t lhs, expr_holder_t2s_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

protected:
#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_tensor_t operator()(T const &lhs) override {                     \
    return dispatch(lhs);                                                      \
  }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_tensor_t operator()(T const &lhs) override {                     \
    return dispatch(lhs);                                                      \
  }
  NUMSIM_CAS_TENSOR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  // tensor_zero × t2s = tensor_zero. m_lhs is already the tensor_zero.
  expr_holder_tensor_t dispatch(tensor_zero const &) noexcept { return m_lhs; }

  expr_holder_tensor_t
  dispatch(tensor_to_scalar_with_tensor_mul const &lhs) noexcept;
  expr_holder_tensor_t dispatch(tensor_scalar_mul const &lhs) noexcept;

  template <typename Expr>
  expr_holder_tensor_t dispatch(Expr const &) noexcept {
    return make_expression<tensor_to_scalar_with_tensor_mul>(m_lhs, m_rhs);
  }

  expr_holder_tensor_t m_lhs;
  expr_holder_t2s_t m_rhs;
};

} // namespace simplifier
} // namespace tensor_with_tensor_to_scalar_detail
} // namespace numsim::cas

#endif // TENSOR_WITH_TENSOR_TO_SCALAR_SIMPLIFIER_MUL_H
