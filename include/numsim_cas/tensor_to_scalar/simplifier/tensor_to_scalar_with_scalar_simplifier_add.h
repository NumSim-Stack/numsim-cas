#ifndef TENSOR_TO_SCALAR_WITH_SCALAR_SIMPLIFIER_ADD_H
#define TENSOR_TO_SCALAR_WITH_SCALAR_SIMPLIFIER_ADD_H

#include "../../functions.h"
#include "../operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_add.h"
#include "../tensor_to_scalar_expression.h"

namespace numsim::cas {
namespace tensor_to_scalar_with_scalar_detail {
namespace simplifier {

template <typename ExprLHS, typename ExprRHS> struct add_default {
  using expr_type = expression_holder<tensor_to_scalar_expression>;

  add_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  //  template<typename Expr>
  //  [[nodiscard]] constexpr inline auto operator()(Expr const&)noexcept{
  //    return get_default();
  //  }

  // protected:
  //   [[nodiscard]] constexpr inline auto get_default()noexcept{
  //     if(m_rhs == m_lhs){
  //       return get_default_same();
  //     }
  //     return get_default_imp();
  //   }

  //  [[nodiscard]] constexpr inline auto get_default_same()noexcept{
  //    auto coef{make_expression<scalar_constant>(2)};
  //    return
  //    make_expression<tensor_to_scalar_with_scalar_mul>(std::move(coef),
  //    m_rhs);
  //  }

  //  [[nodiscard]] constexpr inline auto get_default_imp()noexcept{
  //    auto add_new{make_expression<tensor_to_scalar_add>()};
  //    auto& add{add_new.template get<tensor_to_scalar_add>()};
  //    add.push_back(m_lhs);
  //    add.push_back(m_rhs);
  //    return std::move(add_new);
  //  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class wrapper_scalar_add final : public add_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<tensor_to_scalar_expression>;
  using base = add_default<ExprLHS, ExprRHS>;

  wrapper_scalar_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)) {}

  // scalar + tensor_scalar --> tensor_scalar_with_scalar_add
  constexpr inline expr_type operator()(tensor_to_scalar_expression const &) {
    return make_expression<tensor_to_scalar_with_scalar_add>(m_lhs, m_rhs);
  }

  // scalar + tensor_scalar_with_scalar_add --> tensor_scalar_with_scalar_add
  constexpr inline expr_type
  operator()(tensor_to_scalar_with_scalar_add const &rhs) {
    return make_expression<tensor_to_scalar_with_scalar_add>(
        m_lhs + rhs.expr_lhs(), rhs.expr_rhs());
  }

private:
  using base::m_lhs;
  using base::m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class wrapper_tensor_to_scalar_add final
    : public add_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<tensor_to_scalar_expression>;
  using base = add_default<ExprLHS, ExprRHS>;

  wrapper_tensor_to_scalar_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)) {}

  // tensor_scalar + scalar --> tensor_scalar_with_scalar_add
  constexpr inline expr_type operator()(scalar_expression const &) {
    //{scalar, tensor_to_scalar}
    return make_expression<tensor_to_scalar_with_scalar_add>(m_rhs, m_lhs);
  }

private:
  using base::m_lhs;
  using base::m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class wrapper_tensor_to_scalar_add_add final
    : public add_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<tensor_to_scalar_expression>;
  using base = add_default<ExprLHS, ExprRHS>;

  wrapper_tensor_to_scalar_add_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor_to_scalar_with_scalar_add>()} {}

  // tensor_to_scalar_with_scalar_add + scalar --> tensor_scalar_with_scalar_add
  constexpr inline expr_type operator()(scalar_expression const &) {
    //{scalar, tensor_to_scalar}
    return make_expression<tensor_to_scalar_with_scalar_add>(
        lhs.expr_lhs() + m_rhs, lhs.expr_rhs());
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_to_scalar_with_scalar_add const &lhs;
};

// scalar + tensor_to_scalar --> tensor_to_scalar_with_scalar_add
// scalar + tensor_to_scalar_with_scalar_add -->
// tensor_to_scalar_with_scalar_add tensor_to_scalar + scalar -->
// tensor_scalar_with_scalar_add tensor_to_scalar_with_scalar_add + scalar -->
// tensor_to_scalar_with_scalar_add

template <typename ExprLHS, typename ExprRHS> struct add_base {
  using expr_type = expression_holder<tensor_to_scalar_expression>;

  add_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  template <
      typename Expr,
      std::enable_if_t<std::is_base_of_v<tensor_to_scalar_expression, Expr>,
                       bool> = true>
  constexpr inline expr_type operator()(Expr const &) {
    // return
    // make_expression<tensor_to_scalar_with_scalar_add>(m_rhs,
    // m_lhs);
    auto &expr_rhs{*m_rhs};
    return visit(
        wrapper_tensor_to_scalar_add<ExprLHS, ExprRHS>(
            std::forward<ExprLHS>(m_lhs), std::forward<ExprRHS>(m_rhs)),
        expr_rhs);
  }

  template <
      typename Expr,
      std::enable_if_t<std::is_base_of_v<scalar_expression, Expr>, bool> = true>
  constexpr inline expr_type operator()(Expr const &) {
    // return
    // make_expression<tensor_to_scalar_with_scalar_add>(m_lhs,
    // m_rhs);
    auto &expr_rhs{*m_rhs};
    return visit(
        wrapper_scalar_add<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                             std::forward<ExprRHS>(m_rhs)),
        expr_rhs);
  }

  constexpr inline expr_type
  operator()(tensor_to_scalar_with_scalar_add const &) {
    // return
    // make_expression<tensor_to_scalar_with_scalar_add>(m_rhs,
    // m_lhs);
    auto &expr_rhs{*m_rhs};
    return visit(
        wrapper_tensor_to_scalar_add_add<ExprLHS, ExprRHS>(
            std::forward<ExprLHS>(m_lhs), std::forward<ExprRHS>(m_rhs)),
        expr_rhs);
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};
} // namespace simplifier
} // namespace tensor_to_scalar_with_scalar_detail
} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_WITH_SCALAR_SIMPLIFIER_ADD_H
