#ifndef TENSOR_WITH_SCALAR_SIMPLIFIER_MUL_H
#define TENSOR_WITH_SCALAR_SIMPLIFIER_MUL_H

#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/tensor/operators/scalar/tensor_scalar_mul.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {
namespace tensor_with_scalar_detail {
namespace simplifier {

// lhs := scalar_expression
// rhs := tensor_expression
class mul_base final : public tensor_visitor_return_expr_t {
public:
  using expr_holder_tensor_t = expression_holder<tensor_expression>;
  using expr_holder_scalar_t = expression_holder<scalar_expression>;

  mul_base(expr_holder_scalar_t lhs, expr_holder_tensor_t rhs)
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

  expr_holder_tensor_t dispatch(tensor_scalar_mul const &rhs) noexcept;

  template <typename Expr>
  expr_holder_tensor_t dispatch(Expr const &) noexcept {
    return make_expression<tensor_scalar_mul>(m_lhs, m_rhs);
  }

  expr_holder_scalar_t m_lhs;
  expr_holder_tensor_t m_rhs;
};

} // namespace simplifier
} // namespace tensor_with_scalar_detail
} // namespace numsim::cas

#endif // TENSOR_WITH_SCALAR_SIMPLIFIER_MUL_H
