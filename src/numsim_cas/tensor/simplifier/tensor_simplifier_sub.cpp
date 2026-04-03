#include <numsim_cas/tensor/simplifier/tensor_simplifier_sub.h>
#include <numsim_cas/tensor/tensor_operators.h>

namespace numsim::cas {
namespace tensor_detail {
namespace simplifier {

// negative_sub

negative_sub::negative_sub(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      m_lhs_node{base::m_lhs.template get<tensor_negative>()} {}

// n_ary_sub

n_ary_sub::n_ary_sub(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      m_lhs_node{base::m_lhs.template get<tensor_add>()} {}

// symbol_sub

symbol_sub::symbol_sub(expr_holder_t lhs, expr_holder_t rhs)
    : base(std::move(lhs), std::move(rhs)),
      m_lhs_node{base::m_lhs.template get<tensor>()} {}

/// x-x --> 0
symbol_sub::expr_holder_t symbol_sub::dispatch(tensor const &rhs) {
  if (&m_lhs_node == &rhs) {
    return make_expression<tensor_zero>(rhs.dim(), rhs.rank());
  }
  return get_default();
}

// sub_base

sub_base::sub_base(sub_base::expr_holder_t lhs, sub_base::expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

//  expr_holder_t dispatch(scalar_constant const&){
//    return visit(constant_sub(m_lhs,m_rhs), *m_rhs);
//  }

sub_base::expr_holder_t sub_base::dispatch(tensor_add const &) {
  auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
  n_ary_sub visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

//  expr_holder_t dispatch(scalar_mul const&){
//    return visit(n_ary_mul_sub(m_lhs,m_rhs), *m_rhs);
//  }

sub_base::expr_holder_t sub_base::dispatch(tensor const &) {
  auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
  symbol_sub visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

// 0 - expr
sub_base::expr_holder_t sub_base::dispatch(tensor_zero const &) {
  if (is_same<tensor_zero>(m_rhs))
    return make_expression<tensor_zero>(m_rhs.get().dim(), m_rhs.get().rank());
  return make_expression<tensor_negative>(std::move(m_rhs));
}

// - expr_lhs - expr_rhs --> -(expr_lhs+expr_rhs)
sub_base::expr_holder_t sub_base::dispatch(tensor_negative const &lhs) {
  auto expr{lhs.expr() + std::move(m_rhs)};
  if (expr.is_valid()) {
    return make_expression<tensor_negative>(expr);
  }
  return make_expression<tensor_zero>(lhs.dim(), lhs.rank());
}

} // namespace simplifier
} // namespace tensor_detail
} // namespace numsim::cas
