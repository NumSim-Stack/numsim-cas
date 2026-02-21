#ifndef TENSOR_EVALUATOR_H
#define TENSOR_EVALUATOR_H

#include <algorithm>
#include <cstring>
#include <map>
#include <memory>
#include <numeric>
#include <ranges>

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/expression.h>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/scalar/visitors/scalar_evaluator.h>
#include <numsim_cas/tensor/data/tensor_data.h>
#include <numsim_cas/tensor/data/tensor_data_add.h>
#include <numsim_cas/tensor/data/tensor_data_basis_change.h>
#include <numsim_cas/tensor/data/tensor_data_inner_product.h>
#include <numsim_cas/tensor/data/tensor_data_outer_product.h>
#include <numsim_cas/tensor/data/tensor_data_projector.h>
#include <numsim_cas/tensor/data/tensor_data_scalar_mul.h>
#include <numsim_cas/tensor/data/tensor_data_sub.h>
#include <numsim_cas/tensor/data/tensor_data_unary_wrapper.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_functions.h>

namespace numsim::cas {

template <typename ValueType>
class tensor_evaluator final : public tensor_visitor_const_t {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using data_ptr = std::unique_ptr<tensor_data_base<ValueType>>;

  tensor_evaluator() = default;
  tensor_evaluator(tensor_evaluator const &) = delete;
  tensor_evaluator(tensor_evaluator &&) = delete;
  tensor_evaluator &operator=(tensor_evaluator const &) = delete;

  template <typename ExprBase>
  void set(expression_holder<ExprBase> const &symbol,
           std::shared_ptr<tensor_data_base<ValueType>> val) {
    m_tensor_values[to_base_holder(symbol)] = std::move(val);
  }

  template <typename ExprBase>
  void set_scalar(expression_holder<ExprBase> const &symbol, ValueType val) {
    m_scalar_eval.set(symbol, val);
  }

  data_ptr apply(expr_holder_t const &expr) {
    if (expr.is_valid()) {
      m_current_expr = to_base_holder(expr);
      expr.template get<tensor_visitable_t>().accept(*this);
      return std::move(m_result);
    }
    return nullptr;
  }

  // ─── Symbol ──────────────────────────────────────────────────

  void operator()(tensor const &) override { dispatch_tensor(); }

  // ─── Constants ───────────────────────────────────────────────

  void operator()([[maybe_unused]] tensor_zero const &v) override {
    m_result = make_tensor_data<ValueType>(v.dim(), v.rank());
  }

  void operator()(kronecker_delta const &v) override { eval_identity(v); }

  void operator()(identity_tensor const &v) override { eval_identity(v); }

  // ─── Arithmetic ──────────────────────────────────────────────

  void operator()(tensor_add const &visitable) override {
    auto result =
        make_tensor_data<ValueType>(visitable.dim(), visitable.rank());
    if (visitable.coeff().is_valid()) {
      auto temp = apply(visitable.coeff());
      tensor_data_add<ValueType> add(*result, *temp);
      add.evaluate(visitable.dim(), visitable.rank());
    }
    for (auto const &child : visitable.hash_map() | std::views::values) {
      auto temp = apply(child);
      tensor_data_add<ValueType> add(*result, *temp);
      add.evaluate(visitable.dim(), visitable.rank());
    }
    m_result = std::move(result);
  }

  void operator()(tensor_negative const &v) override {
    eval_unary_tmech<tmech_ops::neg>(v);
  }

  void operator()(tensor_scalar_mul const &visitable) override {
    const auto scalar_val = m_scalar_eval.apply(visitable.expr_lhs());
    auto src = apply(visitable.expr_rhs());
    m_result = make_tensor_data<ValueType>(visitable.dim(), visitable.rank());
    tensor_data_scalar_mul<ValueType> op(*m_result, *src, scalar_val);
    op.evaluate(visitable.dim(), visitable.rank());
  }

  // ─── Products ────────────────────────────────────────────────

  void operator()(inner_product_wrapper const &visitable) override {
    // Short-circuit: detect P:A where P is a known rank-2 projector and A is
    // rank-2
    if (visitable.indices_lhs() == sequence{3, 4} &&
        visitable.indices_rhs() == sequence{1, 2} &&
        visitable.expr_rhs().get().rank() == 2 &&
        is_same<tensor_projector>(visitable.expr_lhs())) {
      auto const &proj = visitable.expr_lhs().template get<tensor_projector>();
      if (proj.acts_on_rank() == 2) {
        auto const &sp = proj.space();
        // Dispatch on known projector types
        if (std::holds_alternative<Symmetric>(sp.perm) &&
            std::holds_alternative<AnyTraceTag>(sp.trace)) {
          eval_projector_unary<tmech_ops::sym>(visitable);
          return;
        }
        if (std::holds_alternative<Skew>(sp.perm) &&
            std::holds_alternative<AnyTraceTag>(sp.trace)) {
          eval_projector_unary<tmech_ops::skew>(visitable);
          return;
        }
        if (std::holds_alternative<Symmetric>(sp.perm) &&
            std::holds_alternative<VolumetricTag>(sp.trace)) {
          eval_projector_unary<tmech_ops::vol>(visitable);
          return;
        }
        if (std::holds_alternative<Symmetric>(sp.perm) &&
            std::holds_alternative<DeviatoricTag>(sp.trace)) {
          eval_projector_unary<tmech_ops::dev>(visitable);
          return;
        }
      }
    }
    // Generic inner product
    auto lhs_data = apply(visitable.expr_lhs());
    auto rhs_data = apply(visitable.expr_rhs());
    m_result = make_tensor_data<ValueType>(visitable.dim(), visitable.rank());
    tensor_data_inner_product<ValueType> ip(*m_result, *lhs_data, *rhs_data,
                                            visitable.indices_lhs().indices(),
                                            visitable.indices_rhs().indices());
    ip.evaluate(visitable.dim(), rhs_data->rank(), lhs_data->rank());
  }

  void operator()(outer_product_wrapper const &visitable) override {
    auto lhs_data = apply(visitable.expr_lhs());
    auto rhs_data = apply(visitable.expr_rhs());
    m_result = make_tensor_data<ValueType>(visitable.dim(), visitable.rank());
    tensor_data_outer_product<ValueType> op(*m_result, *lhs_data, *rhs_data,
                                            visitable.indices_lhs().indices(),
                                            visitable.indices_rhs().indices());
    op.evaluate(visitable.dim(), rhs_data->rank(), lhs_data->rank());
  }

  void operator()(basis_change_imp const &visitable) override {
    auto temp = apply(visitable.expr());
    m_result = make_tensor_data<ValueType>(visitable.dim(), visitable.rank());
    tensor_data_basis_change<ValueType> bc(*m_result, *temp,
                                           visitable.indices().indices());
    bc.evaluate(visitable.dim(), visitable.rank());
  }

  void operator()(simple_outer_product const &visitable) override {
    const auto &children = visitable.data();
    if (children.empty()) {
      m_result = make_tensor_data<ValueType>(visitable.dim(), visitable.rank());
      return;
    }
    auto accumulated = apply(children.front());
    for (std::size_t i = 1; i < children.size(); ++i) {
      auto rhs_data = apply(children[i]);
      const auto lhs_rank = accumulated->rank();
      const auto rhs_rank = rhs_data->rank();
      const auto result_rank = lhs_rank + rhs_rank;
      auto result = make_tensor_data<ValueType>(visitable.dim(), result_rank);
      sequence lhs_seq(lhs_rank), rhs_seq(rhs_rank);
      std::iota(lhs_seq.begin(), lhs_seq.end(), 0);
      std::iota(rhs_seq.begin(), rhs_seq.end(), lhs_rank);
      tensor_data_outer_product<ValueType> op(*result, *accumulated, *rhs_data,
                                              lhs_seq.indices(),
                                              rhs_seq.indices());
      op.evaluate(visitable.dim(), rhs_rank, lhs_rank);
      accumulated = std::move(result);
    }
    m_result = std::move(accumulated);
  }

  void operator()(tensor_mul const &visitable) override {
    const auto &children = visitable.data();
    if (children.empty()) {
      m_result = make_tensor_data<ValueType>(visitable.dim(), visitable.rank());
      return;
    }
    auto accumulated = apply(children.front());
    for (std::size_t i = 1; i < children.size(); ++i) {
      auto rhs_data = apply(children[i]);
      const auto lhs_rank = accumulated->rank();
      const auto rhs_rank = rhs_data->rank();
      const auto result_rank = lhs_rank + rhs_rank - 2; // single contraction
      auto result = make_tensor_data<ValueType>(visitable.dim(), result_rank);
      // contract last index of LHS with first index of RHS
      std::vector<std::size_t> lhs_idx{lhs_rank - 1};
      std::vector<std::size_t> rhs_idx{0};
      tensor_data_inner_product<ValueType> ip(*result, *accumulated, *rhs_data,
                                              lhs_idx, rhs_idx);
      ip.evaluate(visitable.dim(), rhs_rank, lhs_rank);
      accumulated = std::move(result);
    }
    if (visitable.coeff().is_valid()) {
      auto coeff_data = apply(visitable.coeff());
      auto temp =
          make_tensor_data<ValueType>(visitable.dim(), visitable.rank());
      const auto size = compute_size(visitable.dim(), visitable.rank());
      auto *dst = temp->raw_data();
      const auto *lhs = accumulated->raw_data();
      const auto *rhs = coeff_data->raw_data();
      for (std::size_t i = 0; i < size; ++i) {
        dst[i] = lhs[i] * rhs[i];
      }
      accumulated = std::move(temp);
    }
    m_result = std::move(accumulated);
  }

  // ─── Tensor functions (tmech wrappers) ─────────────────────

  void operator()(tensor_pow const &visitable) override {
    auto base_data = apply(visitable.expr_lhs());
    const auto exp_val = m_scalar_eval.apply(visitable.expr_rhs());
    const auto n = static_cast<int>(exp_val);
    const auto d = visitable.dim();
    const auto r = visitable.rank();

    if (n == 0) {
      m_result = make_tensor_data<ValueType>(d, r);
      tensor_data_identity<ValueType> id(*m_result);
      id.evaluate(d, r);
      return;
    }
    if (n == 1) {
      m_result = std::move(base_data);
      return;
    }
    // Repeated contraction for positive integer exponents
    auto accumulated = make_tensor_data<ValueType>(d, r);
    std::memcpy(accumulated->raw_data(), base_data->raw_data(),
                compute_size(d, r) * sizeof(ValueType));
    for (int k = 1; k < std::abs(n); ++k) {
      auto temp = make_tensor_data<ValueType>(d, r);
      std::vector<std::size_t> lhs_idx{r - 1};
      std::vector<std::size_t> rhs_idx{0};
      tensor_data_inner_product<ValueType> ip(*temp, *accumulated, *base_data,
                                              lhs_idx, rhs_idx);
      ip.evaluate(d, r, r);
      accumulated = std::move(temp);
    }
    m_result = std::move(accumulated);
  }

  void operator()(tensor_inv const &v) override {
    eval_unary_tmech<tmech_ops::inv>(v);
  }

  void operator()(tensor_projector const &visitable) override {
    const auto d = visitable.dim();
    const auto r = visitable.rank(); // 2 * acts_on_rank
    m_result = make_tensor_data<ValueType>(d, r);
    tensor_data_projector<ValueType> proj(*m_result, visitable.space());
    proj.evaluate(d, r);
  }

  // ─── Cross-domain ────────────────────────────────────────────

  // f * A where f is tensor_to_scalar (scalar-valued), A is tensor
  // Defined out-of-line after tensor_to_scalar_evaluator is available.
  void operator()(tensor_to_scalar_with_tensor_mul const &visitable) override;

  template <class T> void operator()([[maybe_unused]] T const &) noexcept {
    static_assert(sizeof(T) == 0,
                  "tensor_evaluator: missing overload for this node type");
  }

private:
  // ─── Projector short-circuit: apply tmech op to RHS of inner_product ───

  template <typename Op>
  void eval_projector_unary(inner_product_wrapper const &visitable) {
    auto rhs_data = apply(visitable.expr_rhs());
    m_result = make_tensor_data<ValueType>(visitable.dim(), visitable.rank());
    tensor_data_unary_wrapper<Op, ValueType> op(*m_result, *rhs_data);
    op.evaluate(visitable.dim(), visitable.rank());
  }

  // ─── Generic unary tmech dispatch ───────────────────────────

  template <typename Op, typename Visitable>
  void eval_unary_tmech(Visitable const &visitable) {
    auto temp = apply(visitable.expr());
    m_result = make_tensor_data<ValueType>(visitable.dim(), visitable.rank());
    tensor_data_unary_wrapper<Op, ValueType> op(*m_result, *temp);
    op.evaluate(visitable.dim(), visitable.rank());
  }

  // ─── Identity dispatch via tmech::eye ───────────────────────

  template <typename Visitable> void eval_identity(Visitable const &visitable) {
    m_result = make_tensor_data<ValueType>(visitable.dim(), visitable.rank());
    tensor_data_identity<ValueType> id(*m_result);
    id.evaluate(visitable.dim(), visitable.rank());
  }

  // ─── Symbol dispatch ─────────────────────────────────────────

  void dispatch_tensor() {
    auto it = m_tensor_values.find(m_current_expr);
    if (it == m_tensor_values.end()) {
      throw evaluation_error("tensor_evaluator: symbol not found");
    }
    auto &src = it->second;
    m_result = make_tensor_data<ValueType>(src->dim(), src->rank());
    tensor_data_add<ValueType> add(*m_result, *src);
    add.evaluate(src->dim(), src->rank());
  }

  template <typename ExprBase>
  static expression_holder<expression>
  to_base_holder(expression_holder<ExprBase> const &h) {
    return expression_holder<expression>(
        std::static_pointer_cast<expression>(h.data()));
  }

  static constexpr std::size_t compute_size(std::size_t d,
                                            std::size_t r) noexcept {
    std::size_t size{1};
    for (std::size_t i{0}; i < r; ++i)
      size *= d;
    return size;
  }

  // ─── State ───────────────────────────────────────────────────

  std::map<expression_holder<expression>,
           std::shared_ptr<tensor_data_base<ValueType>>>
      m_tensor_values;
  scalar_evaluator<ValueType> m_scalar_eval;
  data_ptr m_result;
  expression_holder<expression> m_current_expr;
};

} // namespace numsim::cas

// Break circular include: tensor_to_scalar_evaluator includes tensor_evaluator,
// so we include it here (after tensor_evaluator is fully defined) and provide the
// out-of-line implementation of the cross-domain operator.
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_evaluator.h>

namespace numsim::cas {

template <typename ValueType>
void tensor_evaluator<ValueType>::operator()(
    tensor_to_scalar_with_tensor_mul const &visitable) {
  tensor_to_scalar_evaluator<ValueType> t2s_eval;
  for (auto const &[key, val] : m_tensor_values) {
    t2s_eval.set(key, val);
  }
  const auto scalar_val = t2s_eval.apply(visitable.expr_rhs());
  auto src = apply(visitable.expr_lhs());
  const auto dim = src->dim();
  const auto rank = src->rank();
  m_result = make_tensor_data<ValueType>(dim, rank);
  tensor_data_scalar_mul<ValueType> op(*m_result, *src, scalar_val);
  op.evaluate(dim, rank);
}

} // namespace numsim::cas

#endif // TENSOR_EVALUATOR_H
