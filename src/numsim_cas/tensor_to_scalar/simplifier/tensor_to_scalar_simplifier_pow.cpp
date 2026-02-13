#include <numsim_cas/core/operators.h>
#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_pow.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

namespace numsim::cas {
namespace tensor_to_scalar_detail {
namespace simplifier {

// --- pow_pow_visitor virtual function bodies ---
// Defined here so that pow() (from tensor_to_scalar_std.h, included via
// tensor_to_scalar_operators.h) is visible during template instantiation.
#define NUMSIM_LOOP_OVER(T)                                                    \
  pow_pow_visitor::expr_holder_t pow_pow_visitor::operator()(T const &n) {     \
    return this->dispatch(n);                                                  \
  }
NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER

// --- mul_pow_visitor virtual function bodies ---
#define NUMSIM_LOOP_OVER(T)                                                    \
  mul_pow_visitor::expr_holder_t mul_pow_visitor::operator()(T const &n) {     \
    return this->dispatch(n);                                                  \
  }
NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER

// --- pow_base ---
pow_base::pow_base(expr_holder_t lhs, expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

pow_base::expr_holder_t pow_base::dispatch(tensor_to_scalar_pow const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  pow_pow_visitor visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

pow_base::expr_holder_t pow_base::dispatch(tensor_to_scalar_mul const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  mul_pow_visitor visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas
