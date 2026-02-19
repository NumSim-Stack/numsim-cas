#ifndef TENSOR_REBUILD_VISITOR_H
#define TENSOR_REBUILD_VISITOR_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <ranges>

namespace numsim::cas {

class tensor_rebuild_visitor : public tensor_visitor_const_t {
public:
  using tensor_holder_t = expression_holder<tensor_expression>;
  using scalar_holder_t = expression_holder<scalar_expression>;
  using t2s_holder_t = expression_holder<tensor_to_scalar_expression>;

  virtual ~tensor_rebuild_visitor() = default;

  virtual tensor_holder_t apply(tensor_holder_t const &expr) {
    if (expr.is_valid()) {
      m_current = expr;
      expr.get<tensor_visitable_t>().accept(*this);
      return std::move(m_result);
    }
    return expr;
  }

  virtual scalar_holder_t
  apply_scalar(scalar_holder_t const &expr) {
    return expr;
  }

  virtual t2s_holder_t apply_t2s(t2s_holder_t const &expr) {
    return expr;
  }

  // Leaf nodes: return as-is
  void operator()(tensor const &) override { m_result = m_current; }
  void operator()(tensor_zero const &) override {
    m_result = m_current;
  }
  void operator()(kronecker_delta const &) override {
    m_result = m_current;
  }
  void operator()(identity_tensor const &) override {
    m_result = m_current;
  }
  void operator()(tensor_projector const &) override {
    m_result = m_current;
  }

  // Unary tensor -> tensor
  void operator()(tensor_negative const &v) override {
    m_result = -apply(v.expr());
  }

  void operator()(tensor_inv const &v) override {
    m_result = inv(apply(v.expr()));
  }

  void operator()(basis_change_imp const &v) override {
    m_result =
        make_expression<basis_change_imp>(apply(v.expr()), v.indices());
  }

  // Binary tensor x tensor -> tensor
  void operator()(inner_product_wrapper const &v) override {
    m_result = make_expression<inner_product_wrapper>(
        apply(v.expr_lhs()), v.indices_lhs(), apply(v.expr_rhs()),
        v.indices_rhs());
  }

  void operator()(outer_product_wrapper const &v) override {
    m_result = make_expression<outer_product_wrapper>(
        apply(v.expr_lhs()), sequence(v.indices_lhs()), apply(v.expr_rhs()),
        sequence(v.indices_rhs()));
  }

  // Binary tensor x scalar -> tensor (cross-domain!)
  void operator()(tensor_pow const &v) override {
    m_result = make_expression<tensor_pow>(apply(v.expr_lhs()),
                                           apply_scalar(v.expr_rhs()));
  }

  void operator()(tensor_power_diff const &v) override {
    m_result = make_expression<tensor_power_diff>(apply(v.expr_lhs()),
                                                   apply_scalar(v.expr_rhs()));
  }

  // Binary scalar x tensor -> tensor (cross-domain!)
  void operator()(tensor_scalar_mul const &v) override {
    m_result = apply_scalar(v.expr_lhs()) * apply(v.expr_rhs());
  }

  // Binary tensor x t2s -> tensor (cross-domain!)
  void operator()(tensor_to_scalar_with_tensor_mul const &v) override {
    m_result = make_expression<tensor_to_scalar_with_tensor_mul>(
        apply(v.expr_lhs()), apply_t2s(v.expr_rhs()));
  }

  // N-ary tensor ops
  void operator()(tensor_add const &v) override {
    tensor_holder_t result;
    if (v.coeff().is_valid())
      result += apply(v.coeff());
    for (auto &child : v.hash_map() | std::views::values)
      result += apply(child);
    m_result = std::move(result);
  }

  void operator()(tensor_mul const &v) override {
    tensor_holder_t result;
    if (v.coeff().is_valid())
      result *= apply(v.coeff());
    for (auto &child : v.data())
      result *= apply(child);
    m_result = std::move(result);
  }

  void operator()(simple_outer_product const &v) override {
    auto rebuilt = make_expression<simple_outer_product>(
        v.dim(), v.rank());
    for (auto &child : v.data())
      rebuilt.template get<simple_outer_product>().push_back(apply(child));
    m_result = std::move(rebuilt);
  }

protected:
  tensor_holder_t m_current;
  tensor_holder_t m_result;
};

} // namespace numsim::cas

#endif // TENSOR_REBUILD_VISITOR_H
