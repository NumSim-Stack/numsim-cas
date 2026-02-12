#ifndef OUTER_PRODUCT_WRAPPER_H
#define OUTER_PRODUCT_WRAPPER_H

#include <numsim_cas/core/binary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

class outer_product_wrapper final
    : public binary_op<tensor_node_base_t<outer_product_wrapper>,
                       tensor_expression> {
public:
  using base =
      binary_op<tensor_node_base_t<outer_product_wrapper>, tensor_expression>;

  template <typename LHS, typename RHS>
  outer_product_wrapper(LHS &&_lhs, sequence &&_lhs_indices, RHS &&_rhs,
                        sequence &&_rhs_indices)
      : base(std::forward<LHS>(_lhs), std::forward<RHS>(_rhs), _lhs.get().dim(),
             _lhs.get().rank() + _rhs.get().rank()),
        m_lhs_indices(std::move(_lhs_indices)),
        m_rhs_indices(std::move(_rhs_indices)) {}

  const auto &indices_lhs() const noexcept { return m_lhs_indices; }
  const auto &indices_rhs() const noexcept { return m_rhs_indices; }

protected:
  virtual void update_hash_value() const noexcept override {
    hash_combine(base::m_hash_value, base::get_id());
    numsim::cas::hash_combine(base::m_hash_value,
                              base::expr_lhs().get().hash_value());
    numsim::cas::hash_combine(base::m_hash_value,
                              base::expr_rhs().get().hash_value());
    numsim::cas::hash_combine(base::m_hash_value, indices_lhs());
    numsim::cas::hash_combine(base::m_hash_value, indices_rhs());
  }

  sequence m_lhs_indices;
  sequence m_rhs_indices;
};

// template <typename T, typename... Args>
// struct update_hash<
//     numsim::cas::binary_op<numsim::cas::outer_product_wrapper<T>, Args...>> {
//   using type_t =
//       numsim::cas::binary_op<numsim::cas::outer_product_wrapper<T>, Args...>;
//   std::size_t operator()(const type_t &expr) const noexcept {
//     std::size_t seed{0};
//     numsim::cas::hash_combine(seed, type_t::get_id());
//     numsim::cas::hash_combine(seed, expr.expr_lhs().get().hash_value());
//     numsim::cas::hash_combine(seed, expr.expr_rhs().get().hash_value());
//     auto const &derived{
//         static_cast<numsim::cas::outer_product_wrapper<T> const &>(expr)};
//     numsim::cas::hash_combine(seed, derived.indices_lhs());
//     numsim::cas::hash_combine(seed, derived.indices_rhs());
//     return seed;
//   }
// };

} // namespace numsim::cas

#endif // OUTER_PRODUCT_WRAPPER_H
