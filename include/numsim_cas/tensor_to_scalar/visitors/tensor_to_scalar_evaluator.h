#ifndef TENSOR_TO_SCALAR_EVALUATOR_H
#define TENSOR_TO_SCALAR_EVALUATOR_H

#include <cmath>
#include <ranges>

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/expression.h>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/scalar/visitors/scalar_evaluator.h>
#include <numsim_cas/tensor/data/tensor_data.h>
#include <numsim_cas/tensor/data/tensor_data_to_scalar_wrapper.h>
#include <numsim_cas/tensor/visitors/tensor_evaluator.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar_add.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar_mul.h>

namespace numsim::cas {

template <typename ValueType>
class tensor_to_scalar_evaluator final
    : public tensor_to_scalar_visitor_const_t {
public:
  using t2s_holder_t = expression_holder<tensor_to_scalar_expression>;
  using tensor_holder_t = expression_holder<tensor_expression>;

  tensor_to_scalar_evaluator() = default;
  tensor_to_scalar_evaluator(tensor_to_scalar_evaluator const &) = delete;
  tensor_to_scalar_evaluator(tensor_to_scalar_evaluator &&) = delete;
  tensor_to_scalar_evaluator &
  operator=(tensor_to_scalar_evaluator const &) = delete;

  template <typename ExprBase>
  void set(expression_holder<ExprBase> const &symbol,
           std::shared_ptr<tensor_data_base<ValueType>> val) {
    m_tensor_eval.set(symbol, std::move(val));
  }

  template <typename ExprBase>
  void set_scalar(expression_holder<ExprBase> const &symbol, ValueType val) {
    m_scalar_eval.set(symbol, val);
    m_tensor_eval.set_scalar(symbol, val);
  }

  ValueType apply(t2s_holder_t const &expr) {
    if (expr.is_valid()) {
      expr.template get<tensor_to_scalar_visitable_t>().accept(*this);
      return m_result;
    }
    return ValueType{0};
  }

  // ─── Constants ───────────────────────────────────────────────

  void
  operator()([[maybe_unused]] tensor_to_scalar_zero const &) override {
    m_result = ValueType{0};
  }

  void
  operator()([[maybe_unused]] tensor_to_scalar_one const &) override {
    m_result = ValueType{1};
  }

  void operator()(tensor_to_scalar_scalar_wrapper const &v) override {
    m_result = m_scalar_eval.apply(v.expr());
  }

  // ─── Arithmetic ──────────────────────────────────────────────

  void operator()(tensor_to_scalar_negative const &v) override {
    m_result = -apply(v.expr());
  }

  void operator()(tensor_to_scalar_log const &v) override {
    m_result = std::log(apply(v.expr()));
  }

  void operator()(tensor_to_scalar_add const &v) override {
    ValueType result{0};
    if (v.coeff().is_valid()) {
      result += apply(v.coeff());
    }
    for (auto const &child : v.hash_map() | std::views::values) {
      result += apply(child);
    }
    m_result = result;
  }

  void operator()(tensor_to_scalar_mul const &v) override {
    ValueType result{1};
    if (v.coeff().is_valid()) {
      result = apply(v.coeff());
    }
    for (auto const &child : v.hash_map() | std::views::values) {
      result *= apply(child);
    }
    m_result = result;
  }

  void operator()(tensor_to_scalar_pow const &v) override {
    m_result = std::pow(apply(v.expr_lhs()), apply(v.expr_rhs()));
  }

  // ─── Tensor → scalar operations ─────────────────────────────

  void operator()(tensor_trace const &v) override {
    m_result = eval_tensor_to_scalar<tmech_ops::trace_op>(v.expr());
  }

  void operator()(tensor_det const &v) override {
    m_result = eval_tensor_to_scalar<tmech_ops::det_op>(v.expr());
  }

  void operator()(tensor_norm const &v) override {
    m_result = eval_tensor_to_scalar<tmech_ops::norm_op>(v.expr());
  }

  void operator()(tensor_dot const &v) override {
    m_result = eval_tensor_to_scalar<tmech_ops::dcontract_self_op>(v.expr());
  }

  void operator()(tensor_inner_product_to_scalar const &v) override {
    auto lhs_data = m_tensor_eval.apply(v.expr_lhs());
    auto rhs_data = m_tensor_eval.apply(v.expr_rhs());
    const auto dim = lhs_data->dim();
    const auto rank = lhs_data->rank();
    tensor_data_dcontract_wrapper<ValueType> op(*lhs_data, *rhs_data);
    m_result = op.evaluate(dim, rank);
  }

  template <class T> void operator()([[maybe_unused]] T const &) noexcept {
    static_assert(sizeof(T) == 0,
                  "tensor_to_scalar_evaluator: missing overload for this "
                  "node type");
  }

private:
  template <typename Op>
  ValueType eval_tensor_to_scalar(tensor_holder_t const &tensor_expr) {
    auto data = m_tensor_eval.apply(tensor_expr);
    const auto dim = data->dim();
    const auto rank = data->rank();
    tensor_data_to_scalar_wrapper<Op, ValueType> op(*data);
    return op.evaluate(dim, rank);
  }

  tensor_evaluator<ValueType> m_tensor_eval;
  scalar_evaluator<ValueType> m_scalar_eval;
  ValueType m_result{};
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_EVALUATOR_H
