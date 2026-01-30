#ifndef OUTER_PRODUCT_WRAPPER_H
#define OUTER_PRODUCT_WRAPPER_H

#include "../../binary_op.h"
#include "../../numsim_cas_type_traits.h"
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace numsim::cas {

class outer_product_wrapper final
    : public binary_op<outer_product_wrapper, tensor_expression,
                       tensor_expression> {
public:
  using base =
      binary_op<outer_product_wrapper, tensor_expression, tensor_expression>;

  template <typename LHS, typename RHS>
  outer_product_wrapper(LHS &&_lhs, sequence &&_lhs_indices, RHS &&_rhs,
                        sequence &&_rhs_indices)
      : base(std::forward<LHS>(_lhs), std::forward<RHS>(_rhs),
             call_tensor::dim(_lhs),
             call_tensor::rank(_lhs) + call_tensor::rank(_rhs)),
        m_lhs_indices(std::move(_lhs_indices)),
        m_rhs_indices(std::move(_rhs_indices)) {
    init();
  }

  const auto &indices_lhs() const noexcept { return m_lhs_indices; }
  const auto &indices_rhs() const noexcept { return m_rhs_indices; }

protected:
  virtual void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
    numsim::cas::hash_combine(base::m_hash_value,
                              base::expr_lhs().get().hash_value());
    numsim::cas::hash_combine(base::m_hash_value,
                              base::expr_rhs().get().hash_value());
    numsim::cas::hash_combine(base::m_hash_value, indices_lhs());
    numsim::cas::hash_combine(base::m_hash_value, indices_rhs());
  }

  void init() {
    std::for_each(m_lhs_indices.begin(), m_lhs_indices.end(),
                  [](std::size_t &index) { index -= 1; });
    std::for_each(m_rhs_indices.begin(), m_rhs_indices.end(),
                  [](std::size_t &index) { index -= 1; });
    // tensor_expression &lhs{*this->m_lhs};
    // tensor_expression &rhs{*this->m_rhs};
  }

  sequence m_lhs_indices;
  sequence m_rhs_indices;
};

template <typename T, typename... Args>
struct update_hash<
    numsim::cas::binary_op<numsim::cas::outer_product_wrapper<T>, Args...>> {
  using type_t =
      numsim::cas::binary_op<numsim::cas::outer_product_wrapper<T>, Args...>;
  std::size_t operator()(const type_t &expr) const noexcept {
    std::size_t seed{0};
    numsim::cas::hash_combine(seed, type_t::get_id());
    numsim::cas::hash_combine(seed, expr.expr_lhs().get().hash_value());
    numsim::cas::hash_combine(seed, expr.expr_rhs().get().hash_value());
    auto const &derived{
        static_cast<numsim::cas::outer_product_wrapper<T> const &>(expr)};
    numsim::cas::hash_combine(seed, derived.indices_lhs());
    numsim::cas::hash_combine(seed, derived.indices_rhs());
    return seed;
  }
};

} // namespace numsim::cas

#endif // OUTER_PRODUCT_WRAPPER_H
