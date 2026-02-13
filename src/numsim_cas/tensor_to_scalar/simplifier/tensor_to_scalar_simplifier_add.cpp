#include <numsim_cas/core/operators.h>
#include <numsim_cas/functions.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_add.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_domain_traits.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

namespace numsim::cas {
namespace tensor_to_scalar_detail {
namespace simplifier {

// ------------------------------------------------------------
// n_ary_add — domain-specific scalar_wrapper dispatch
// ------------------------------------------------------------
n_ary_add::expr_holder_t
n_ary_add::dispatch(tensor_to_scalar_scalar_wrapper const &rhs) {
  using Traits = domain_traits<tensor_to_scalar_expression>;
  // Numeric scalar_wrappers: use generic coefficient-based dispatch
  if (Traits::try_numeric(m_rhs)) {
    return algo::dispatch(rhs);
  }
  // Non-numeric: find matching in hash_map, combine or push_back
  auto expr_add{make_expression<tensor_to_scalar_add>(lhs)};
  auto &add{expr_add.get<tensor_to_scalar_add>()};
  auto pos{add.hash_map().find(m_rhs)};
  if (pos != add.hash_map().end()) {
    auto combined{pos->second + m_rhs};
    add.hash_map().erase(pos);
    add.push_back(std::move(combined));
    return expr_add;
  }
  add.push_back(m_rhs);
  return expr_add;
}

// ------------------------------------------------------------
// add_base
// ------------------------------------------------------------
add_base::add_base(expr_holder_t lhs, expr_holder_t rhs)
    : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

add_base::expr_holder_t add_base::dispatch(tensor_to_scalar_zero const &) {
  return std::move(m_rhs);
}

add_base::expr_holder_t add_base::dispatch(tensor_to_scalar_add const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  n_ary_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t
add_base::dispatch(tensor_to_scalar_negative const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  negative_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

template <typename Type>
add_base::expr_holder_t add_base::dispatch(Type const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  add_default_visitor visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas
