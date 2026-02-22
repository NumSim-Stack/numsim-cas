#ifndef TENSOR_TO_SCALAR_REBUILD_VISITOR_H
#define TENSOR_TO_SCALAR_REBUILD_VISITOR_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_operators.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_std.h>
#include <ranges>

namespace numsim::cas {

class tensor_to_scalar_rebuild_visitor
    : public tensor_to_scalar_visitor_const_t {
public:
  using t2s_holder_t = expression_holder<tensor_to_scalar_expression>;
  using scalar_holder_t = expression_holder<scalar_expression>;
  using tensor_holder_t = expression_holder<tensor_expression>;

  virtual ~tensor_to_scalar_rebuild_visitor() = default;

  virtual t2s_holder_t apply(t2s_holder_t const &expr) {
    if (expr.is_valid()) {
      m_current = expr;
      expr.get<tensor_to_scalar_visitable_t>().accept(*this);
      return std::move(m_result);
    }
    return expr;
  }

  virtual scalar_holder_t apply_scalar(scalar_holder_t const &expr) {
    return expr;
  }

  virtual tensor_holder_t apply_tensor(tensor_holder_t const &expr) {
    return expr;
  }

  // Leaf nodes
  void operator()(tensor_to_scalar_zero const &) override {
    m_result = m_current;
  }

  void operator()(tensor_to_scalar_one const &) override {
    m_result = m_current;
  }

  // Unary t2s -> t2s
  void operator()(tensor_to_scalar_negative const &v) override {
    m_result = -apply(v.expr());
  }

  void operator()(tensor_to_scalar_log const &v) override {
    m_result = log(apply(v.expr()));
  }

  void operator()(tensor_to_scalar_exp const &v) override {
    m_result = exp(apply(v.expr()));
  }

  void operator()(tensor_to_scalar_sqrt const &v) override {
    m_result = sqrt(apply(v.expr()));
  }

  // Unary scalar -> t2s (cross-domain!)
  void operator()(tensor_to_scalar_scalar_wrapper const &v) override {
    m_result = make_expression<tensor_to_scalar_scalar_wrapper>(
        apply_scalar(v.expr()));
  }

  // Unary tensor -> t2s (cross-domain!)
  void operator()(tensor_trace const &v) override {
    m_result = trace(apply_tensor(v.expr()));
  }

  void operator()(tensor_dot const &v) override {
    m_result = dot(apply_tensor(v.expr()));
  }

  void operator()(tensor_det const &v) override {
    m_result = det(apply_tensor(v.expr()));
  }

  void operator()(tensor_norm const &v) override {
    m_result = norm(apply_tensor(v.expr()));
  }

  // Binary t2s x t2s -> t2s
  void operator()(tensor_to_scalar_pow const &v) override {
    m_result = pow(apply(v.expr_lhs()), apply(v.expr_rhs()));
  }

  // Binary tensor x tensor -> t2s (cross-domain!)
  void operator()(tensor_inner_product_to_scalar const &v) override {
    m_result = make_expression<tensor_inner_product_to_scalar>(
        apply_tensor(v.expr_lhs()), v.indices_lhs(), apply_tensor(v.expr_rhs()),
        v.indices_rhs());
  }

  // N-ary t2s ops
  void operator()(tensor_to_scalar_add const &v) override {
    t2s_holder_t result;
    if (v.coeff().is_valid())
      result += apply(v.coeff());
    for (auto &child : v.symbol_map() | std::views::values)
      result += apply(child);
    m_result = std::move(result);
  }

  void operator()(tensor_to_scalar_mul const &v) override {
    t2s_holder_t result;
    if (v.coeff().is_valid())
      result *= apply(v.coeff());
    for (auto &child : v.symbol_map() | std::views::values)
      result *= apply(child);
    m_result = std::move(result);
  }

protected:
  t2s_holder_t m_current;
  t2s_holder_t m_result;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_REBUILD_VISITOR_H
