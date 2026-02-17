#include <numsim_cas/core/operators.h>
#include <numsim_cas/functions.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_add.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_domain_traits.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>

namespace numsim::cas {
namespace tensor_to_scalar_detail {
namespace simplifier {

// --- n_ary_add::dispatch(T) template body ---
// Defined here so that operator+ (from tensor_to_scalar_operators.h) is visible.
template <typename T>
n_ary_add::expr_holder_t n_ary_add::dispatch(T const &) {
  auto expr_add{make_expression<tensor_to_scalar_add>(this->lhs)};
  auto &add{expr_add.template get<tensor_to_scalar_add>()};
  auto pos{add.hash_map().find(this->m_rhs)};
  if (pos != add.hash_map().end()) {
    auto combined{pos->second + this->m_rhs};
    add.hash_map().erase(pos);
    add.push_back(std::move(combined));
    return expr_add;
  }
  add.push_back(this->m_rhs);
  return expr_add;
}

// --- add_default_visitor virtual function bodies ---
// Defined here so that operator+ (from tensor_to_scalar_operators.h) is visible
// during template instantiation.
#define NUMSIM_LOOP_OVER(T)                                                    \
  add_default_visitor::expr_holder_t add_default_visitor::operator()(           \
      T const &n) {                                                            \
    return this->dispatch(n);                                                  \
  }
NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER

// --- n_ary_add virtual function bodies ---
#define NUMSIM_LOOP_OVER(T)                                                    \
  n_ary_add::expr_holder_t n_ary_add::operator()(T const &n) {                \
    return this->dispatch(n);                                                  \
  }
NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER

// --- negative_add virtual function bodies ---
#define NUMSIM_LOOP_OVER(T)                                                    \
  negative_add::expr_holder_t negative_add::operator()(T const &n) {           \
    return this->dispatch(n);                                                  \
  }
NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER

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

// --- n_ary_mul_add virtual function bodies ---
#define NUMSIM_LOOP_OVER(T)                                                    \
  n_ary_mul_add::expr_holder_t n_ary_mul_add::operator()(T const &n) {         \
    return this->dispatch(n);                                                  \
  }
NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER

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

add_base::expr_holder_t
add_base::dispatch(tensor_to_scalar_scalar_wrapper const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  constant_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t
add_base::dispatch(tensor_to_scalar_one const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  one_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t
add_base::dispatch(tensor_to_scalar_mul const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  n_ary_mul_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas
