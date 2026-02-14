#include <numsim_cas/core/operators.h>
#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_mul.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

namespace numsim::cas {
namespace tensor_to_scalar_detail {
namespace simplifier {

// wrapper_tensor_to_scalar_mul_mul::wrapper_tensor_to_scalar_mul_mul(expr_holder_t
// lhs, expr_holder_t rhs)
//     : base(std::move(lhs), std::move(rhs)),
//       lhs{base::m_lhs.template get<tensor_to_scalar_with_scalar_mul>()} {}

//        // tensor_to_scalar_with_scalar_mul * tensor_to_scalar_with_scalar_mul
//        -->
//        // tensor_to_scalar_with_scalar_mul
// [[nodiscard]] wrapper_tensor_to_scalar_mul_mul::expr_holder_t
// wrapper_tensor_to_scalar_mul_mul::dispatch(tensor_to_scalar_with_scalar_mul
// const &rhs) {
//   return make_expression<tensor_to_scalar_with_scalar_mul>(
//       lhs.expr_lhs() * rhs.expr_lhs(), lhs.expr_rhs() * rhs.expr_rhs());
// }

//        // tensor_to_scalar_with_scalar_mul * tensor_to_scalar -->
//        // tensor_to_scalar_with_scalar_mul
// template <typename Expr>
// [[nodiscard]] wrapper_tensor_to_scalar_mul_mul::expr_holder_t
// wrapper_tensor_to_scalar_mul_mul::dispatch(Expr const &) {
//   return make_expression<tensor_to_scalar_with_scalar_mul>(
//       lhs.expr_lhs(), m_rhs * lhs.expr_rhs());
// }

mul_base::mul_base(expr_holder_t lhs, expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

[[nodiscard]] mul_base::expr_holder_t
mul_base::dispatch(tensor_to_scalar_mul const &) {
  auto new_mul{copy_expression<tensor_to_scalar_mul>(std::move(m_lhs))};
  auto &mul{new_mul.template get<tensor_to_scalar_mul>()};
  auto pos{mul.hash_map().find(m_rhs)};
  if (pos != mul.hash_map().end()) {
    auto expr{pos->second * m_rhs};
    mul.hash_map().erase(pos);
    mul.push_back(std::move(expr));
    return new_mul;
  }
  mul.push_back(m_rhs);
  return new_mul;
}

//        // tensor_scalar_with_scalar_mul * tensor_scalar -->
//        // tensor_scalar_with_scalar_mul
// [[nodiscard]] mul_base::expr_holder_t
// mul_base::dispatch(tensor_to_scalar_with_scalar_mul const &) {
//   auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
//   wrapper_tensor_to_scalar_mul_mul visitor(std::move(m_lhs),
//   std::move(m_rhs)); return _rhs.accept(visitor);
// }

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas
