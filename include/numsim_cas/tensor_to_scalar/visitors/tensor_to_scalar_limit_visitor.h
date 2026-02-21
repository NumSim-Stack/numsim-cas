#ifndef TENSOR_TO_SCALAR_LIMIT_VISITOR_H
#define TENSOR_TO_SCALAR_LIMIT_VISITOR_H

#include <numsim_cas/core/limit_algebra.h>
#include <numsim_cas/core/limit_result.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar_add.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar_mul.h>

namespace numsim::cas {

enum class dependency_mode { exact_match, tensor_dependency };

class tensor_to_scalar_limit_visitor final
    : public tensor_to_scalar_visitor_const_t,
      protected limit_algebra {
public:
  using t2s_holder_t = expression_holder<tensor_to_scalar_expression>;
  using tensor_holder_t = expression_holder<tensor_expression>;

  // Mode 1: exact sub-expression match (e.g., det(F) is the limit var)
  tensor_to_scalar_limit_visitor(t2s_holder_t const &limit_var,
                                 limit_target target);

  // Mode 2: any T2S expression depending on tensor F
  tensor_to_scalar_limit_visitor(tensor_holder_t const &tensor_var,
                                 limit_target target);

  limit_result apply(t2s_holder_t const &expr);

  // T2S functions
  void operator()(tensor_trace const &) override;
  void operator()(tensor_dot const &) override;
  void operator()(tensor_det const &) override;
  void operator()(tensor_norm const &) override;
  void operator()(tensor_inner_product_to_scalar const &) override;

  // Arithmetic
  void operator()(tensor_to_scalar_add const &) override;
  void operator()(tensor_to_scalar_mul const &) override;
  void operator()(tensor_to_scalar_negative const &) override;
  void operator()(tensor_to_scalar_pow const &) override;
  void operator()(tensor_to_scalar_log const &) override;

  // Constants
  void operator()(tensor_to_scalar_zero const &) override;
  void operator()(tensor_to_scalar_one const &) override;
  void operator()(tensor_to_scalar_scalar_wrapper const &) override;

private:
  bool depends_on_limit_var(t2s_holder_t const &expr) const;

  dependency_mode m_mode;
  t2s_holder_t m_limit_var_t2s;
  tensor_holder_t m_tensor_var;
  limit_target m_target;
  limit_result m_result;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_LIMIT_VISITOR_H
