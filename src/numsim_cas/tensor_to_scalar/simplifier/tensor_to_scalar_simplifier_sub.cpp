#include <numsim_cas/core/operators.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_sub.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_scalar_wrapper.h>

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

// --- n_ary_sub::dispatch(T) generic template body ---
// Defined here so that operator- (from tensor_to_scalar_operators.h) is visible.
template <typename T>
n_ary_sub::expr_holder_t n_ary_sub::dispatch(T const &) {
  auto expr_add{make_expression<tensor_to_scalar_add>(this->lhs)};
  auto &add{expr_add.template get<tensor_to_scalar_add>()};
  // Direct match: (sum with expr) - expr → combine
  auto pos{add.hash_map().find(this->m_rhs)};
  if (pos != add.hash_map().end()) {
    auto combined{pos->second - this->m_rhs};
    add.hash_map().erase(pos);
    add.push_back(std::move(combined));
    return expr_add;
  }
  add.push_back(-this->m_rhs);
  return expr_add;
}

// --- n_ary_sub virtual function bodies ---
#define NUMSIM_LOOP_OVER(T)                                                    \
  n_ary_sub::expr_holder_t n_ary_sub::operator()(T const &n) {                \
    return this->dispatch(n);                                                  \
  }
NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER

// ------------------------------------------------------------
// n_ary_sub — domain-specific scalar_wrapper dispatch
// ------------------------------------------------------------
n_ary_sub::expr_holder_t
n_ary_sub::dispatch(tensor_to_scalar_scalar_wrapper const &rhs) {
  using Traits = domain_traits<tensor_to_scalar_expression>;
  // Numeric scalar_wrappers: use generic coefficient-based dispatch
  if (Traits::try_numeric(m_rhs)) {
    return algo::dispatch(rhs);
  }
  // Non-numeric: find any existing scalar_wrapper child and merge
  auto expr_add{make_expression<tensor_to_scalar_add>(lhs)};
  auto &add{expr_add.get<tensor_to_scalar_add>()};
  auto wrappers = get_all<tensor_to_scalar_scalar_wrapper>(add);
  if (!wrappers.empty()) {
    auto &existing_w = wrappers[0].get<tensor_to_scalar_scalar_wrapper>();
    auto &rhs_w = m_rhs.get<tensor_to_scalar_scalar_wrapper>();
    auto merged = existing_w.expr() - rhs_w.expr();
    add.hash_map().erase(wrappers[0]);
    auto wrapper = make_expression<tensor_to_scalar_scalar_wrapper>(std::move(merged));
    auto val = Traits::try_numeric(wrapper);
    if (!val || *val != scalar_number{0}) {
      add.push_back(std::move(wrapper));
    }
    return expr_add;
  }
  add.push_back(-m_rhs);
  return expr_add;
}

// ------------------------------------------------------------
// constant_sub — domain-specific scalar_wrapper dispatch
// ------------------------------------------------------------
constant_sub::expr_holder_t
constant_sub::dispatch(tensor_to_scalar_scalar_wrapper const &rhs) {
  using Traits = domain_traits<tensor_to_scalar_expression>;
  auto lhs_val = Traits::try_numeric(m_lhs);
  auto rhs_val = Traits::try_numeric(m_rhs);
  if (lhs_val && rhs_val) {
    return algo::dispatch(rhs);
  }
  // Non-numeric: unwrap both scalars, sub in scalar domain, re-wrap
  auto &lhs_w = m_lhs.get<tensor_to_scalar_scalar_wrapper>();
  auto &rhs_w = m_rhs.get<tensor_to_scalar_scalar_wrapper>();
  auto result = lhs_w.expr() - rhs_w.expr();
  auto wrapper = make_expression<tensor_to_scalar_scalar_wrapper>(std::move(result));
  auto val = Traits::try_numeric(wrapper);
  if (val && *val == scalar_number{0})
    return Traits::zero();
  return wrapper;
}

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
