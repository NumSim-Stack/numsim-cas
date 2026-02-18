#ifndef TENSOR_TO_SCALAR_DIFFERENTIATION_H
#define TENSOR_TO_SCALAR_DIFFERENTIATION_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/diff.h>
#include <numsim_cas/tensor/kronecker_delta.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_zero.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

// Forward declare the diff CPOs used recursively
expression_holder<tensor_expression>
tag_invoke(detail::diff_fn, std::type_identity<tensor_expression>,
           std::type_identity<tensor_expression>,
           expression_holder<tensor_expression> const &,
           expression_holder<tensor_expression> const &);

expression_holder<tensor_expression>
tag_invoke(detail::diff_fn, std::type_identity<tensor_to_scalar_expression>,
           std::type_identity<tensor_expression>,
           expression_holder<tensor_to_scalar_expression> const &,
           expression_holder<tensor_expression> const &);

} // namespace numsim::cas

namespace numsim::cas {

class tensor_to_scalar_differentiation final
    : public tensor_to_scalar_visitor_const_t {
public:
  using tensor_holder_t = expression_holder<tensor_expression>;
  using t2s_holder_t = expression_holder<tensor_to_scalar_expression>;

  explicit tensor_to_scalar_differentiation(tensor_holder_t const &arg)
      : m_arg(arg) {
    m_dim = arg.get().dim();
    m_rank_arg = arg.get().rank();
    m_I = make_expression<kronecker_delta>(m_dim);
  }

  tensor_to_scalar_differentiation(
      tensor_to_scalar_differentiation const &) = delete;
  tensor_to_scalar_differentiation(
      tensor_to_scalar_differentiation &&) = delete;
  const tensor_to_scalar_differentiation &
  operator=(tensor_to_scalar_differentiation const &) = delete;

  [[nodiscard]] tensor_holder_t apply(t2s_holder_t const &expr) {
    m_result = tensor_holder_t{};
    if (expr.is_valid()) {
      m_expr = expr;
      expr.get<tensor_to_scalar_visitable_t>().accept(*this);
    }
    if (!m_result.is_valid()) {
      return make_expression<tensor_zero>(m_dim, m_rank_arg);
    }
    return m_result;
  }

  // All operator() methods declared here, defined in .cpp
  void operator()(tensor_to_scalar_zero const &visitable) override;
  void operator()(tensor_to_scalar_one const &visitable) override;
  void operator()(tensor_to_scalar_scalar_wrapper const &visitable) override;
  void operator()(tensor_to_scalar_negative const &visitable) override;
  void operator()(tensor_to_scalar_add const &visitable) override;
  void operator()(tensor_to_scalar_mul const &visitable) override;
  void operator()(tensor_to_scalar_pow const &visitable) override;
  void operator()(tensor_to_scalar_log const &visitable) override;
  void operator()(tensor_trace const &visitable) override;
  void operator()(tensor_dot const &visitable) override;
  void operator()(tensor_norm const &visitable) override;
  void operator()(tensor_det const &visitable) override;
  void operator()(tensor_inner_product_to_scalar const &visitable) override;

private:
  tensor_holder_t const &m_arg;
  std::size_t m_dim{0};
  std::size_t m_rank_arg{0};
  tensor_holder_t m_result;
  t2s_holder_t m_expr;
  tensor_holder_t m_I;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_DIFFERENTIATION_H
