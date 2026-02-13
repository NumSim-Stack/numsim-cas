#include <numsim_cas/core/operators.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/simplifier/scalar_simplifier_sub.h>

namespace numsim::cas {
namespace simplifier {

sub_base::sub_base(expr_holder_t lhs, expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

sub_base::expr_holder_t sub_base::dispatch(scalar_constant const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  constant_sub visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

sub_base::expr_holder_t sub_base::dispatch(scalar_add const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  n_ary_sub visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

sub_base::expr_holder_t sub_base::dispatch(scalar_mul const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  n_ary_mul_sub visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

sub_base::expr_holder_t sub_base::dispatch(scalar const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  symbol_sub visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

sub_base::expr_holder_t sub_base::dispatch(scalar_one const &) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  scalar_one_sub visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

template <typename Type>
sub_base::expr_holder_t sub_base::dispatch([[maybe_unused]] Type const &rhs) {
  auto &_rhs{m_rhs.template get<scalar_visitable_t>()};
  sub_default_visitor visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

// 0 - expr
sub_base::expr_holder_t sub_base::dispatch(scalar_zero const &) {
  return make_expression<scalar_negative>(std::move(m_rhs));
}

// - expr_lhs - expr_rhs --> -(expr_lhs+expr_rhs)
sub_base::expr_holder_t sub_base::dispatch(scalar_negative const &lhs) {
  return make_expression<scalar_negative>(lhs.expr() + std::move(m_rhs));
}

} // namespace simplifier
} // namespace numsim::cas
