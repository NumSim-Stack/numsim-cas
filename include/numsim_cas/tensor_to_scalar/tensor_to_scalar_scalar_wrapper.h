#ifndef TENSOR_TO_SCALAR_SCALAR_WRAPPER_H
#define TENSOR_TO_SCALAR_SCALAR_WRAPPER_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/scalar/scalar_assume.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

class tensor_to_scalar_scalar_wrapper final
    : public unary_op<
          tensor_to_scalar_node_base_t<tensor_to_scalar_scalar_wrapper>,
          scalar_expression> {
public:
  using base =
      unary_op<tensor_to_scalar_node_base_t<tensor_to_scalar_scalar_wrapper>,
               scalar_expression>;
  using base::base;

  tensor_to_scalar_scalar_wrapper(
      tensor_to_scalar_scalar_wrapper &&data) noexcept
      : base(std::move(static_cast<base &&>(data))) {}
  tensor_to_scalar_scalar_wrapper(tensor_to_scalar_scalar_wrapper const &data)
      : base(data) {}
  ~tensor_to_scalar_scalar_wrapper() override = default;

  const tensor_to_scalar_scalar_wrapper &
  operator=(tensor_to_scalar_scalar_wrapper &&) = delete;

  // Transparent forwarding of Symbol classification: this wrapper is the
  // bridge that lets a named scalar (e.g. scalar("x")) appear in a t2s
  // expression. A user who has only a `holder<t2s_expression>` referring
  // to such a wrapped scalar should still be able to call
  // `.assumption(...)` — the wrapped child IS a Symbol. Forward through.
  // For non-Symbol payloads (constants, compound scalar expressions),
  // this returns the payload's own answer (typically false).
  [[nodiscard]] bool is_symbol() const noexcept override {
    return this->expr().is_valid() && this->expr().get().is_symbol();
  }

  friend bool operator<(tensor_to_scalar_scalar_wrapper const &lhs,
                        tensor_to_scalar_scalar_wrapper const &rhs) {
    return lhs.hash_value() < rhs.hash_value();
  }

  friend bool operator>(tensor_to_scalar_scalar_wrapper const &lhs,
                        tensor_to_scalar_scalar_wrapper const &rhs) {
    return rhs < lhs;
  }

  friend bool operator==(tensor_to_scalar_scalar_wrapper const &lhs,
                         tensor_to_scalar_scalar_wrapper const &rhs) {
    return lhs.hash_value() == rhs.hash_value();
  }

  friend bool operator!=(tensor_to_scalar_scalar_wrapper const &lhs,
                         tensor_to_scalar_scalar_wrapper const &rhs) {
    return !(lhs == rhs);
  }

  void update_hash_value() const noexcept override {
    base::m_hash_value = 0;
    hash_combine(base::m_hash_value, base::get_id());
    if (this->expr().is_valid()) {
      hash_combine(base::m_hash_value, this->expr().get().hash_value());
    }
  }
};

// Transparent apply_assumption forwarding for the t2s scalar wrapper.
// The holder-level require_symbol passed via is_symbol() forwarding above;
// here we unwrap to the inner scalar holder and dispatch via its
// apply_assumption overload (defined in scalar_assume.h). Constrained to
// the same set of valid fact tags as the scalar overload — keeps the
// assumption_fact_for concept's diagnostic-quality guarantees consistent
// across domains.
//
// Architect step-5 review gap: without this, a user holding
// `expression_holder<tensor_to_scalar_expression>` over a wrapped scalar
// Symbol would pass the holder-level guard but fail ADL on the per-fact
// dispatch — compile error for a perfectly valid call. Fixes the
// inconsistency between the is_symbol() forwarder (says "Symbol") and
// the assumption() dispatch (says "no overload").
template <typename Tag>
requires detail::is_numeric_assumption_tag<std::remove_cvref_t<Tag>>::value
inline void apply_assumption(expression_holder<tensor_to_scalar_expression> &h,
                             Tag &&tag) {
  auto inner = h.template get<tensor_to_scalar_scalar_wrapper>().expr();
  apply_assumption(inner, std::forward<Tag>(tag));
}

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_SCALAR_WRAPPER_H
