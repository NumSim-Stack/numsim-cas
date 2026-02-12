#ifndef TENSOR_TO_SCALAR_SIMPLIFIER_ADD_H
#define TENSOR_TO_SCALAR_SIMPLIFIER_ADD_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_add.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_mul.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {
namespace tensor_to_scalar_detail {
namespace simplifier {

template <typename Derived>
class add_default : public tensor_to_scalar_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<tensor_to_scalar_expression>;

  add_default(expr_holder_t lhs, expr_holder_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

  template <typename Expr>
  [[nodiscard]] expr_holder_t dispatch(Expr const &) noexcept {
    return get_default();
  }

  // // tensor_to_scalar + tensor_scalar_with_scalar_add -->
  // // tensor_scalar_with_scalar_add
  // [[nodiscard]] expr_holder_t
  // dispatch(tensor_to_scalar_with_scalar_add const &rhs) noexcept {
  //   return make_expression<tensor_to_scalar_with_scalar_add>(
  //       rhs.expr_lhs(), rhs.expr_rhs() + m_lhs);
  // }

protected:
#define NUMSIM_ADD_OVR(T)                                                      \
  expr_holder_t operator()(T const &n) override {                              \
    if constexpr (std::is_void_v<Derived>) {                                   \
      return dispatch(n);                                                      \
    } else {                                                                   \
      return static_cast<Derived *>(this)->dispatch(n);                        \
    }                                                                          \
  }
  NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_ADD_OVR, NUMSIM_ADD_OVR)
#undef NUMSIM_ADD_OVR

  [[nodiscard]] expr_holder_t get_default() noexcept {
    if (m_rhs == m_lhs) {
      return get_default_same();
    }
    return get_default_imp();
  }

  [[nodiscard]] expr_holder_t get_default_same() noexcept {
    auto coef{make_expression<tensor_to_scalar_scalar_wrapper>(
        make_expression<scalar_constant>(2))};
    auto expr{make_expression<tensor_to_scalar_mul>()};
    expr.get<tensor_to_scalar_mul>().push_back(std::move(m_rhs));
    expr.get<tensor_to_scalar_mul>().set_coeff(coef);
    return expr;
  }

  [[nodiscard]] expr_holder_t get_default_imp() noexcept {
    auto add_new{make_expression<tensor_to_scalar_add>()};
    auto &add{add_new.template get<tensor_to_scalar_add>()};
    add.push_back(m_lhs);
    add.push_back(m_rhs);
    return add_new;
  }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

class n_ary_add final : public add_default<n_ary_add> {
public:
  using expr_holder_t = expression_holder<tensor_to_scalar_expression>;
  using base = add_default<n_ary_add>;
  using base::dispatch;

  n_ary_add(expr_holder_t lhs, expr_holder_t rhs);

  // merge two expression
  expr_holder_t dispatch(tensor_to_scalar_add const &rhs);

  expr_holder_t dispatch(tensor_to_scalar_scalar_wrapper const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_to_scalar_add const &lhs;
};

class add_base final : public tensor_to_scalar_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<tensor_to_scalar_expression>;

  add_base(expr_holder_t lhs, expr_holder_t rhs);

protected:
#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST,
                                        NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  template <typename Type> expr_holder_t dispatch(Type const &);

  expr_holder_t dispatch(tensor_to_scalar_add const &);

  // // tensor_scalar_with_scalar_add + tensor_scalar -->
  // // tensor_scalar_with_scalar_add
  // expr_holder_t
  // dispatch(tensor_to_scalar_with_scalar_add const &) {
  //   auto &expr_rhs{*m_rhs};
  //   return visit(
  //       wrapper_tensor_to_scalar_add_add(
  //           std::move(m_lhs), std::move(m_rhs)),
  //       expr_rhs);
  // }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas

#endif // TENSOR_SCALAR_SIMPLIFIER_ADD_H
