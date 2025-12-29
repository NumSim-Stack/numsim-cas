#ifndef TENSOR_TO_SCALAR_SIMPLIFIER_MUL_H
#define TENSOR_TO_SCALAR_SIMPLIFIER_MUL_H

#include "../../operators.h"
#include "../operators/tensor_to_scalar/tensor_to_scalar_mul.h"
#include "../tensor_to_scalar_expression.h"
#include "../tensor_to_scalar_pow.h"

namespace numsim::cas {
namespace tensor_to_scalar_detail {
namespace simplifier {

template <typename ExprLHS, typename ExprRHS> struct mul_default {
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;

  mul_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  [[nodiscard]] constexpr inline expr_type get_default() {
    if (m_lhs.get().hash_value() == m_rhs.get().hash_value()) {
      return get_default_same();
    }
    return get_default_imp();
  }

  template <typename Expr>
  [[nodiscard]] constexpr inline expr_type operator()(Expr const &) {
    return get_default();
  }

  // tensor_to_scalar * tensor_to_scalar_with_scalar_mul -->
  // tensor_to_scalar_with_scalar_mul
  [[nodiscard]] constexpr inline expr_type
  operator()(tensor_to_scalar_with_scalar_mul<value_type> const &rhs) noexcept {
    return make_expression<tensor_to_scalar_with_scalar_mul<value_type>>(
        rhs.expr_lhs(), rhs.expr_rhs() * m_lhs);
  }

protected:
  [[nodiscard]] constexpr inline expr_type get_default_same() {
    auto coeff{make_expression<scalar_constant<value_type>>(2)};
    return make_expression<
        tensor_to_scalar_pow_with_scalar_exponent<value_type>>(
        m_rhs, std::move(coeff));
  }

  [[nodiscard]] constexpr inline expr_type get_default_imp() {
    auto mul_new{make_expression<tensor_to_scalar_mul<value_type>>()};
    auto &mul{mul_new.template get<tensor_to_scalar_mul<value_type>>()};
    mul.push_back(m_lhs);
    mul.push_back(m_rhs);
    return std::move(mul_new);
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

// tensor_scalar_with_scalar_add + tensor_to_scalar -->
// tensor_to_scalar_with_scalar_add tensor_to_scalar +
// tensor_to_scalar_with_scalar_add --> tensor_to_scalar_with_scalar_add
// tensor_scalar_with_scalar_add + tensor_scalar_with_scalar_add -->
// tensor_to_scalar_with_scalar_add
template <typename ExprLHS, typename ExprRHS>
class wrapper_tensor_to_scalar_mul_mul final
    : public mul_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;
  using base = mul_default<ExprLHS, ExprRHS>;

  wrapper_tensor_to_scalar_mul_mul(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs
                .template get<tensor_to_scalar_with_scalar_mul<value_type>>()} {
  }

  // tensor_to_scalar_with_scalar_mul * tensor_to_scalar_with_scalar_mul -->
  // tensor_to_scalar_with_scalar_mul
  [[nodiscard]] constexpr inline expr_type
  operator()(tensor_to_scalar_with_scalar_mul<value_type> const &rhs) {
    return make_expression<tensor_to_scalar_with_scalar_mul<value_type>>(
        lhs.expr_lhs() * rhs.expr_lhs(), lhs.expr_rhs() * rhs.expr_rhs());
  }

  // tensor_to_scalar_with_scalar_mul * tensor_to_scalar -->
  // tensor_to_scalar_with_scalar_mul
  template <typename Expr>
  [[nodiscard]] constexpr inline expr_type operator()(Expr const &) {
    return make_expression<tensor_to_scalar_with_scalar_mul<value_type>>(
        lhs.expr_lhs(), m_rhs * lhs.expr_rhs());
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_to_scalar_with_scalar_mul<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS> struct mul_base {
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;

  mul_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  [[nodiscard]] constexpr inline expr_type
  operator()(tensor_to_scalar_mul<value_type> const &) {
    auto new_mul{copy_expression<tensor_to_scalar_mul<value_type>>(
        std::forward<ExprLHS>(m_lhs))};
    auto &mul{new_mul.template get<tensor_to_scalar_mul<value_type>>()};
    auto pos{mul.hash_map().find(m_rhs)};
    if (pos != mul.hash_map().end()) {
      auto expr{pos->second * m_rhs};
      mul.hash_map().erase(pos);
      mul.push_back(std::move(expr));
      return new_mul;
    }
    mul.push_back(m_rhs);
    return new_mul;
  }

  template <typename Type>
  [[nodiscard]] constexpr inline expr_type operator()(Type const &) {
    auto &expr_rhs{*m_rhs};
    return visit(mul_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                               std::forward<ExprRHS>(m_rhs)),
                 expr_rhs);
  }

  // tensor_scalar_with_scalar_mul * tensor_scalar -->
  // tensor_scalar_with_scalar_mul
  [[nodiscard]] constexpr inline expr_type
  operator()(tensor_to_scalar_with_scalar_mul<value_type> const &) {
    auto &expr_rhs{*m_rhs};
    return visit(
        wrapper_tensor_to_scalar_mul_mul<ExprLHS, ExprRHS>(
            std::forward<ExprLHS>(m_lhs), std::forward<ExprRHS>(m_rhs)),
        expr_rhs);
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_SIMPLIFIER_MUL_H
