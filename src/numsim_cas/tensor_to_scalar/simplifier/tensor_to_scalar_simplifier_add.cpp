#include <numsim_cas/core/operators.h>
#include <numsim_cas/functions.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_add.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

namespace numsim::cas {
namespace tensor_to_scalar_detail {
namespace simplifier {

n_ary_add::n_ary_add(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      lhs{base::m_lhs.template get<tensor_to_scalar_add>()} {}

// merge two expression
n_ary_add::expr_holder_t n_ary_add::dispatch(tensor_to_scalar_add const &rhs) {
  auto expr{make_expression<tensor_to_scalar_add>()};
  auto &add{expr.template get<tensor_to_scalar_add>()};
  merge_add(lhs, rhs, add);
  return expr;
}

n_ary_add::expr_holder_t
n_ary_add::dispatch(tensor_to_scalar_scalar_wrapper const &rhs) {
  auto expr{make_expression<tensor_to_scalar_add>(lhs)};
  auto scalar_wrappers{get_all<tensor_to_scalar_scalar_wrapper>(lhs)};
  expression_holder<scalar_expression> result;
  for (const auto &scalar : scalar_wrappers) {
    result += scalar.get<tensor_to_scalar_scalar_wrapper>().expr();
    expr.get<tensor_to_scalar_add>().hash_map().erase(scalar);
  }
  auto temp = result + rhs.expr();
  return std::move(expr) * std::move(temp);
}

// // tensor_scalar_with_scalar_add + tensor_to_scalar -->
// // tensor_to_scalar_with_scalar_add tensor_to_scalar +
// // tensor_to_scalar_with_scalar_add --> tensor_to_scalar_with_scalar_add
// // tensor_scalar_with_scalar_add + tensor_scalar_with_scalar_add -->
// // tensor_to_scalar_with_scalar_add
// class wrapper_tensor_to_scalar_add_add final
//     : public add_default<wrapper_tensor_to_scalar_add_add> {
// public:
//   using expr_holder_t = expression_holder<tensor_to_scalar_expression>;
//   using base = add_default<wrapper_tensor_to_scalar_add_add>;

//   wrapper_tensor_to_scalar_add_add(expr_holder_t lhs, expr_holder_t rhs)
//       : base(std::move(lhs), std::move(rhs)),
//         lhs{base::m_lhs.template get<tensor_to_scalar_with_scalar_add>()} {}

//   // tensor_scalar_with_scalar_add + tensor_scalar_with_scalar_add -->
//   // tensor_scalar_with_scalar_add
//   expr_holder_t
//   dispatch(tensor_to_scalar_with_scalar_add const &rhs) {
//     return make_expression<tensor_to_scalar_with_scalar_add>(
//         lhs.expr_lhs() + rhs.expr_lhs(), lhs.expr_rhs() + rhs.expr_rhs());
//   }

//   // tensor_scalar_with_scalar_add + tensor_to_scalar -->
//   // tensor_scalar_with_scalar_add
//   template <typename Expr> expr_holder_t dispatch(Expr const &) {
//     return make_expression<tensor_to_scalar_with_scalar_add>(
//         lhs.expr_lhs(), m_rhs + lhs.expr_rhs());
//   }

add_base::add_base(expr_holder_t lhs, expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

template <typename Type>
add_base::expr_holder_t add_base::dispatch(Type const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  add_default<void> visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(tensor_to_scalar_add const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  n_ary_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas
