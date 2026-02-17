#include <numsim_cas/core/operators.h>
#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_sub.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

namespace numsim::cas {
namespace tensor_to_scalar_detail {
namespace simplifier {

// --- negative_sub virtual function bodies ---
// Defined here so that operator+ (from tensor_to_scalar_operators.h) is visible
// during template instantiation.
#define NUMSIM_LOOP_OVER(T)                                                    \
  negative_sub::expr_holder_t negative_sub::operator()(T const &n) {           \
    return this->dispatch(n);                                                  \
  }
NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER

// --- constant_sub virtual function bodies ---
#define NUMSIM_LOOP_OVER(T)                                                    \
  constant_sub::expr_holder_t constant_sub::operator()(T const &n) {           \
    return this->dispatch(n);                                                  \
  }
NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER

// --- n_ary_sub virtual function bodies ---
#define NUMSIM_LOOP_OVER(T)                                                    \
  n_ary_sub::expr_holder_t n_ary_sub::operator()(T const &n) {                \
    return this->dispatch(n);                                                  \
  }
NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER

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

sub_base::expr_holder_t
sub_base::dispatch(tensor_to_scalar_scalar_wrapper const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  constant_sub visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

sub_base::expr_holder_t
sub_base::dispatch(tensor_to_scalar_one const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  one_sub visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

sub_base::expr_holder_t
sub_base::dispatch(tensor_to_scalar_mul const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  n_ary_mul_sub visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas
