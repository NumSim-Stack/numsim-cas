#ifndef TENSOR_PROJECTOR_SIMPLIFIER_H
#define TENSOR_PROJECTOR_SIMPLIFIER_H

#include <numsim_cas/tensor/visitors/tensor_rebuild_visitor.h>

namespace numsim::cas {

/// Projector algebra simplifier.
///
/// Traverses the expression tree and applies projection-specific rules:
///
/// Contraction rules (inner_product_wrapper):
///   P : P         → P   (idempotent, same space)
///   P_i : P_j     → 0   (orthogonal spaces)
///   P_sub : P_par → P_sub (subspace)
///
/// Addition rules (tensor_add):
///   P_vol:A + P_dev:A → P_sym:A
///   P_sym:A + P_skew:A → A  (identity)
class tensor_projector_simplifier : public tensor_rebuild_visitor {
public:
  using tensor_holder_t = expression_holder<tensor_expression>;

  tensor_holder_t apply(tensor_holder_t const &expr) override;

  void operator()(inner_product_wrapper const &v) override;
  void operator()(tensor_add const &v) override;
};

} // namespace numsim::cas

#endif // TENSOR_PROJECTOR_SIMPLIFIER_H
