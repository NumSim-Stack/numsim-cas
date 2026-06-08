#ifndef TENSOR_TO_SCALAR_DIFFERENTIATION_WRT_SCALAR_H
#define TENSOR_TO_SCALAR_DIFFERENTIATION_WRT_SCALAR_H

// Differentiates a tensor-to-scalar expression with respect to a
// SCALAR variable. Companion to tensor_differentiation_wrt_scalar
// (#275) and tensor_to_scalar_differentiation (which differentiates
// w.r.t. a tensor variable).
//
// Result type: tensor_to_scalar_expression. A scalar-valued
// quantity that may contain tensor invariants (trace, norm, det,
// dot) is naturally typed as t2s; pure-scalar results are wrapped
// via tensor_to_scalar_scalar_wrapper. Closes #285.
//
// Each node applies the chain/product rule, routing:
//   - a scalar coefficient's derivative through the existing
//     diff(scalar, scalar),
//   - a tensor sub-expression's scalar-derivative through #275's
//     diff(tensor, scalar),
//   - tensor reductions (trace, norm, dot, det) via their known
//     derivatives composed with the inner tensor's scalar
//     derivative.

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/diff.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/tensor/identity_tensor.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

// Forward declare the CPO this visitor implements.
expression_holder<tensor_to_scalar_expression>
tag_invoke(detail::diff_fn, std::type_identity<tensor_to_scalar_expression>,
           std::type_identity<scalar_expression>,
           expression_holder<tensor_to_scalar_expression> const &,
           expression_holder<scalar_expression> const &);

class tensor_to_scalar_differentiation_wrt_scalar final
    : public tensor_to_scalar_visitor_const_t {
public:
  using tensor_holder_t = expression_holder<tensor_expression>;
  using t2s_holder_t = expression_holder<tensor_to_scalar_expression>;
  using scalar_holder_t = expression_holder<scalar_expression>;

  explicit tensor_to_scalar_differentiation_wrt_scalar(
      scalar_holder_t const &arg)
      : m_arg(arg) {}

  tensor_to_scalar_differentiation_wrt_scalar(
      tensor_to_scalar_differentiation_wrt_scalar const &) = delete;
  tensor_to_scalar_differentiation_wrt_scalar(
      tensor_to_scalar_differentiation_wrt_scalar &&) = delete;
  const tensor_to_scalar_differentiation_wrt_scalar &
  operator=(tensor_to_scalar_differentiation_wrt_scalar const &) = delete;

  [[nodiscard]] t2s_holder_t apply(t2s_holder_t const &expr);

  // All operator() declared here, defined in .cpp.
  void operator()(tensor_to_scalar_zero const &visitable) override;
  void operator()(tensor_to_scalar_one const &visitable) override;
  void operator()(tensor_to_scalar_scalar_wrapper const &visitable) override;
  void operator()(tensor_to_scalar_negative const &visitable) override;
  void operator()(tensor_to_scalar_add const &visitable) override;
  void operator()(tensor_to_scalar_mul const &visitable) override;
  void operator()(tensor_to_scalar_pow const &visitable) override;
  void operator()(tensor_to_scalar_log const &visitable) override;
  void operator()(tensor_to_scalar_exp const &visitable) override;
  void operator()(tensor_to_scalar_sqrt const &visitable) override;
  void operator()(tensor_trace const &visitable) override;
  void operator()(tensor_dot const &visitable) override;
  void operator()(tensor_norm const &visitable) override;
  void operator()(tensor_det const &visitable) override;
  void operator()(tensor_inner_product_to_scalar const &visitable) override;
  void operator()(tensor_to_scalar_if_then_else const &visitable) override;

private:
  scalar_holder_t const &m_arg;
  t2s_holder_t m_result;
  t2s_holder_t m_expr;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_DIFFERENTIATION_WRT_SCALAR_H
