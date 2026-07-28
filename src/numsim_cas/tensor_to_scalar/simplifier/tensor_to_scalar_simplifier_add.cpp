#include <numsim_cas/core/operators.h>
#include <numsim_cas/core/simplifier/simplifier_common.h>
#include <numsim_cas/functions.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/simplifier/tensor_to_scalar_simplifier_add.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_domain_traits.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_scalar_wrapper.h>

namespace numsim::cas {
namespace tensor_to_scalar_detail {
namespace simplifier {

// --- add_default_visitor virtual function bodies ---
// Defined here so that operator+ (from tensor_to_scalar_operators.h) is visible
// during template instantiation.
#define NUMSIM_LOOP_OVER(T)                                                    \
  add_default_visitor::expr_holder_t add_default_visitor::operator()(          \
      T const &n) {                                                            \
    return this->dispatch(n);                                                  \
  }
NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_LOOP_OVER, NUMSIM_LOOP_OVER)
#undef NUMSIM_LOOP_OVER

// --- n_ary_add virtual function bodies ---
#define NUMSIM_LOOP_OVER(T)                                                    \
  n_ary_add::expr_holder_t n_ary_add::operator()(T const &n) {                 \
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
  // Non-numeric: find any existing scalar_wrapper child and merge
  using Traits = domain_traits<tensor_to_scalar_expression>;
  auto expr_add{make_expression<tensor_to_scalar_add>(lhs)};
  auto &add{expr_add.get<tensor_to_scalar_add>()};
  auto wrappers = get_all<tensor_to_scalar_scalar_wrapper>(add);
  if (!wrappers.empty()) {
    auto &existing_w = wrappers[0].get<tensor_to_scalar_scalar_wrapper>();
    auto &rhs_w = m_rhs.get<tensor_to_scalar_scalar_wrapper>();
    auto merged = existing_w.expr() + rhs_w.expr();
    add.symbol_map().erase(wrappers[0]);
    auto wrapper =
        make_expression<tensor_to_scalar_scalar_wrapper>(std::move(merged));
    auto val = Traits::try_numeric(wrapper);
    if (!val) {
      add.push_back(std::move(wrapper));
    } else if (*val != scalar_number{0}) {
      // numeric result belongs in the coeff (canonical form), not a child
      if (add.coeff().is_valid()) {
        auto new_coeff = add.coeff() + wrapper;
        auto cval = Traits::try_numeric(new_coeff);
        if (cval && *cval == scalar_number{0}) {
          add.coeff().free();
        } else {
          add.set_coeff(std::move(new_coeff));
        }
      } else {
        add.set_coeff(std::move(wrapper));
      }
    }
    // round-2 review: a cancelled wrapper left an uncollapsed
    // single-child add with a stale cached hash
    return detail::finalize_add<Traits>(std::move(expr_add));
  }
  add.push_back(m_rhs);
  return expr_add;
}

// ------------------------------------------------------------
// n_ary_add — negative scalar_wrapper normalization (round-5 sweep)
// ------------------------------------------------------------
n_ary_add::expr_holder_t
n_ary_add::dispatch(tensor_to_scalar_negative const &rhs) {
  if (is_same<tensor_to_scalar_scalar_wrapper>(rhs.expr())) {
    auto &w = rhs.expr().template get<tensor_to_scalar_scalar_wrapper>();
    m_rhs = make_expression<tensor_to_scalar_scalar_wrapper>(-w.expr());
    return dispatch(m_rhs.template get<tensor_to_scalar_scalar_wrapper>());
  }
  return algo::dispatch(rhs);
}

// ------------------------------------------------------------
// constant_add — domain-specific scalar_wrapper dispatch
// ------------------------------------------------------------
constant_add::expr_holder_t
constant_add::dispatch(tensor_to_scalar_scalar_wrapper const &rhs) {
  using Traits = domain_traits<tensor_to_scalar_expression>;
  auto lhs_val = Traits::try_numeric(m_lhs);
  auto rhs_val = Traits::try_numeric(m_rhs);
  if (lhs_val && rhs_val) {
    return algo::dispatch(rhs);
  }
  // Non-numeric: unwrap both scalars, add in scalar domain, re-wrap
  auto &lhs_w = m_lhs.get<tensor_to_scalar_scalar_wrapper>();
  auto &rhs_w = m_rhs.get<tensor_to_scalar_scalar_wrapper>();
  auto result = lhs_w.expr() + rhs_w.expr();
  auto wrapper =
      make_expression<tensor_to_scalar_scalar_wrapper>(std::move(result));
  auto val = Traits::try_numeric(wrapper);
  if (val && *val == scalar_number{0})
    return Traits::zero();
  return wrapper;
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

add_base::expr_holder_t add_base::dispatch(tensor_to_scalar_negative const &) {
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

add_base::expr_holder_t add_base::dispatch(tensor_to_scalar_one const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  one_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

add_base::expr_holder_t add_base::dispatch(tensor_to_scalar_mul const &) {
  auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
  n_ary_mul_add visitor(std::move(m_lhs), std::move(m_rhs));
  return _rhs.accept(visitor);
}

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas
