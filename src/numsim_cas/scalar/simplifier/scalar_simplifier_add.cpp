#include <numsim_cas/core/operators.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/simplifier/scalar_simplifier_add.h>

namespace numsim::cas {
namespace simplifier {

// ------------------------------------------------------------
// add_base
// ------------------------------------------------------------
add_base::add_base(expr_holder_t lhs, expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

add_base::expr_holder_t add_base::dispatch(scalar_constant const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  constant_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar_one const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  add_scalar_one visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar_add const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  n_ary_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar_mul const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  n_ary_mul_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  symbol_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar_negative const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  add_negative visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(scalar_zero const &) {
  return std::move(m_rhs);
}

} // namespace simplifier
} // namespace numsim::cas
