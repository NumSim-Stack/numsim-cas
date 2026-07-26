#ifndef TENSOR_SIMPLIFIER_ADD_H
#define TENSOR_SIMPLIFIER_ADD_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/core/simplifier/simplifier_add.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_domain_traits.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

using tensor_traits = domain_traits<tensor_expression>;

namespace simplifier {
namespace tensor_detail {

// Deep-compared like-term fold: (c1*T)+(c2*T) -> (c1+c2)*T, T+(c*T) -> (1+c)*T.
// hash(c*T)==hash(T) by design, so hash equality alone must never merge.
// Defined in tensor_simplifier_add.cpp where the scalar/tensor operators
// are visible.
[[nodiscard]] expression_holder<tensor_expression>
try_merge_scalar_mul(expression_holder<tensor_expression> const &a,
                     expression_holder<tensor_expression> const &b);

// Tensor-specific add base: inherits generic add_dispatch for members/zero
// dispatch, but overrides get_default() and other dispatches that require
// tensor-specific handling (mul_type is void, add_type needs dim/rank).
template <typename Derived>
class add_default : public tensor_visitor_return_expr_t,
                    public detail::add_dispatch<tensor_traits, void> {
  using algo = detail::add_dispatch<tensor_traits, void>;

public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using algo::algo;

  [[nodiscard]] expr_holder_t get_default() {
    if (algo::m_lhs == algo::m_rhs) {
      auto constant{make_expression<scalar_constant>(2)};
      return make_expression<tensor_scalar_mul>(std::move(constant),
                                                std::move(algo::m_rhs));
    }
    // Like terms: (c1*T)+(c2*T) → (c1+c2)*T, T+(c*T) → (1+c)*T
    if (auto merged = try_merge_scalar_mul(algo::m_lhs, algo::m_rhs);
        merged.is_valid()) {
      return merged;
    }
    auto add_new{make_expression<tensor_add>(algo::m_lhs.get().dim(),
                                             algo::m_rhs.get().rank())};
    auto &add{add_new.template get<tensor_add>()};
    add.push_back(algo::m_lhs);
    add.push_back(algo::m_rhs);
    return add_new;
  }

  [[nodiscard]] expr_holder_t dispatch(tensor_negative const &expr) {
    if (algo::m_lhs == expr.expr()) {
      return tensor_traits::zero(algo::m_lhs);
    }
    return get_default();
  }

  // non-add + add --> swap so add is LHS (round-11 review: get_default
  // nested the rhs add as a single child, so A+(B+C) != (A+B)+C).
  // n_ary_add overrides this with the real merge, so no swap loop.
  [[nodiscard]] expr_holder_t dispatch(tensor_add const &) {
    return algo::m_rhs + algo::m_lhs;
  }

  template <typename Expr> [[nodiscard]] expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

  [[nodiscard]] expr_holder_t dispatch(tensor_zero const &) {
    return algo::m_lhs;
  }

protected:
#define NUMSIM_ADD_OVR(T)                                                      \
  expr_holder_t operator()(T const &n) override {                              \
    if constexpr (std::is_void_v<Derived>) {                                   \
      return dispatch(n);                                                      \
    } else {                                                                   \
      return static_cast<Derived *>(this)->dispatch(n);                        \
    }                                                                          \
  }
  NUMSIM_CAS_TENSOR_NODE_LIST(NUMSIM_ADD_OVR, NUMSIM_ADD_OVR)
#undef NUMSIM_ADD_OVR

  using algo::m_lhs;
  using algo::m_rhs;
};

// Thin wrapper: LHS is add (n-ary sum)
class n_ary_add final : public add_default<n_ary_add> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = add_default<n_ary_add>;
  using base::dispatch;
  using base::get_default;

  n_ary_add(expr_holder_t lhs, expr_holder_t rhs);

  template <typename Expr>
  [[nodiscard]] expr_holder_t dispatch([[maybe_unused]] Expr const &rhs);

  [[nodiscard]] expr_holder_t dispatch(tensor_add const &rhs);

  [[nodiscard]] expr_holder_t dispatch(tensor_negative const &rhs);

protected:
  using base::m_lhs;
  using base::m_rhs;
  tensor_add const &m_lhs_node;
};

// Thin wrapper: LHS is symbol
class symbol_add final : public add_default<symbol_add> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = add_default<symbol_add>;
  using base::dispatch;
  using base::get_default;

  symbol_add(expr_holder_t lhs, expr_holder_t rhs);

  [[nodiscard]] expr_holder_t dispatch(tensor const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor const &m_lhs_node;
};

// Thin wrapper: LHS is tensor_scalar_mul (tensor-specific, no generic equiv)
class tensor_scalar_mul_add final : public add_default<tensor_scalar_mul_add> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = add_default<tensor_scalar_mul_add>;
  using base::dispatch;
  using base::get_default;

  tensor_scalar_mul_add(expr_holder_t lhs, expr_holder_t rhs);

  // scalar_expr * lhs + rhs
  template <typename Expr>
  [[nodiscard]] expr_holder_t dispatch(Expr const &rhs);

  // scalar_expr_lhs * lhs + scalar_expr_rhs * rhs
  [[nodiscard]] expr_holder_t dispatch(tensor_scalar_mul const &rhs);

  // (s*T) + (-T) → (s-1)*T
  [[nodiscard]] expr_holder_t dispatch(tensor_negative const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_scalar_mul const &m_lhs_node;
};

// Thin wrapper: LHS is negative
class add_negative final : public add_default<add_negative> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = add_default<add_negative>;
  using base::dispatch;
  using base::get_default;

  add_negative(expr_holder_t lhs, expr_holder_t rhs);

  [[nodiscard]] expr_holder_t dispatch(tensor_negative const &rhs);

  [[nodiscard]] expr_holder_t dispatch(tensor_scalar_mul const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_negative const &m_lhs_node;
};

// Dispatcher: analyzes LHS type and creates appropriate specialized visitor
class add_base final : public tensor_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<tensor_expression>;

  add_base(expr_holder_t lhs, expr_holder_t rhs);

#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_TENSOR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  [[nodiscard]] expr_holder_t dispatch(tensor_zero const &);

  [[nodiscard]] expr_holder_t dispatch(tensor_add const &);

  [[nodiscard]] expr_holder_t dispatch(tensor_scalar_mul const &);

  [[nodiscard]] expr_holder_t dispatch(tensor_negative const &);

  [[nodiscard]] expr_holder_t dispatch(tensor const &);

  [[nodiscard]] expr_holder_t dispatch(inner_product_wrapper const &);

  template <typename Type> [[nodiscard]] expr_holder_t dispatch(Type const &) {
    auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
    add_default<void> visitor(std::move(m_lhs), std::move(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

} // namespace tensor_detail
} // namespace simplifier

} // namespace numsim::cas
#endif // TENSOR_SIMPLIFIER_ADD_H
