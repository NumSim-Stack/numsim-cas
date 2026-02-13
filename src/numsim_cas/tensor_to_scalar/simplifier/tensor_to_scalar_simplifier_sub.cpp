#include <numsim_cas/core/operators.h>
#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_sub.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

namespace numsim::cas {
namespace tensor_to_scalar_detail {
namespace simplifier {

sub_base::sub_base(expr_holder_t lhs, expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

sub_base::expr_holder_t sub_base::dispatch(tensor_to_scalar_add const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  n_ary_sub visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

// 0 - expr
sub_base::expr_holder_t sub_base::dispatch(tensor_to_scalar_zero const &) {
  return make_expression<tensor_to_scalar_negative>(std::move(m_rhs));
}

// -expr_lhs - expr_rhs --> -(expr_lhs+expr_rhs)
sub_base::expr_holder_t
sub_base::dispatch(tensor_to_scalar_negative const &lhs) {
  return make_expression<tensor_to_scalar_negative>(lhs.expr() +
                                                    std::move(m_rhs));
}

template <typename Type>
sub_base::expr_holder_t sub_base::dispatch(Type const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  sub_default_visitor visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas
