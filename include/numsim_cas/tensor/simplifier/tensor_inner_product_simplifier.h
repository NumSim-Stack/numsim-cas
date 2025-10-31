#ifndef TENSOR_INNER_PRODUCT_SIMPLIFIER_H
#define TENSOR_INNER_PRODUCT_SIMPLIFIER_H

#include "../../numsim_cas_type_traits.h"

namespace numsim::cas {
namespace detail {
template <typename ValueType, template <typename> class TypeLHS,
          template <typename> class TypeRHS>
struct otimes_check {
  using lhs_type = TypeLHS<ValueType>;
  using rhs_type = TypeRHS<ValueType>;

  otimes_check(sequence &&lhs, sequence &&rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

  constexpr inline auto
  check(expression_holder<tensor_expression<ValueType>> const &expr) const {
    if (is_same<outer_product_wrapper<ValueType>>(expr)) {
      return inner_check(expr);
    }
    return false;
  }

  constexpr inline auto inner_check(
      expression_holder<tensor_expression<ValueType>> const &expr) const {
    const auto &type{expr.template get<outer_product_wrapper<ValueType>>()};
    if (type.indices_lhs() == m_lhs && type.indices_rhs() == m_rhs) {
      if (is_same<lhs_type>(type.expr_lhs()) &&
          is_same<rhs_type>(type.expr_rhs())) {
        return true;
      }
    }
    return false;
  }

  sequence m_lhs;
  sequence m_rhs;
};
} // namespace detail
} // namespace numsim::cas

namespace numsim::cas {

template <typename ExprLHS, typename ExprRHS>
class inner_product_simplifier_default {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;

  inner_product_simplifier_default(ExprLHS &&lhs, sequence &&sequence_lhs,
                                   ExprRHS &&rhs, sequence &&sequence_rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)),
        m_seq_lhs(std::move(sequence_lhs)), m_seq_rhs(std::move(sequence_rhs)) {
  }

  // expr * I
  constexpr inline auto operator()(kronecker_delta<value_type> const &) {
    if (m_lhs.get().rank() == 2 && sequence{2} == m_seq_lhs &&
        sequence{1} == m_seq_rhs) {
      return std::forward<ExprLHS>(m_lhs);
    }
    return get_default();
  }

  // expr [*] otimesu(I,I)
  constexpr inline auto operator()(outer_product_wrapper<value_type> const &) {
    const detail::otimes_check<value_type, kronecker_delta, kronecker_delta>
        check(sequence{0, 2}, sequence{1, 3});
    if (((m_seq_lhs == sequence{1, 2} && m_seq_rhs == sequence{1, 2}) ||
         (m_seq_lhs == sequence{1, 2} && m_seq_rhs == sequence{3, 4})) &&
        m_lhs.get().rank() == 2 && check.inner_check(m_rhs)) {
      return std::forward<ExprLHS>(m_lhs);
    }
    return get_default();
  }

  template <typename T> constexpr inline auto operator()(T const &) {
    return get_default();
  }

protected:
  constexpr inline auto get_default() {
    return make_expression<inner_product_wrapper<value_type>>(
        std::forward<ExprLHS>(m_lhs), std::move(m_seq_lhs),
        std::forward<ExprRHS>(m_rhs), std::move(m_seq_rhs));
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
  sequence m_seq_lhs;
  sequence m_seq_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class inner_product_simplifier_kronecker_delta final
    : public inner_product_simplifier_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using base = inner_product_simplifier_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_default;

  inner_product_simplifier_kronecker_delta(ExprLHS &&lhs,
                                           sequence &&sequence_lhs,
                                           ExprRHS &&rhs,
                                           sequence &&sequence_rhs)
      : base(std::forward<ExprLHS>(lhs), std::move(sequence_lhs),
             std::forward<ExprRHS>(rhs), std::move(sequence_rhs)) {}

  template <typename Expr> constexpr inline auto operator()(Expr const &rhs) {
    // I * expr --> expr
    if (rhs.rank() == 2 && sequence{2} == m_seq_lhs &&
        sequence{1} == m_seq_rhs) {
      return std::forward<ExprRHS>(m_rhs);
    }
    return get_default();
  }

  constexpr inline auto
  operator()(outer_product_wrapper<value_type> const &rhs) {
    // I:otimes(epxr, expr) = I_ij (expr_ij, expr_kl) = trace(expr)*expr
    // if(rhs.expr_lhs().get().rank() == 2 && m_seq_lhs == sequence{1,2} &&
    // m_seq_lhs == sequence{1,2}){
    //   if(rhs.indices_lhs() == sequence{1,2}){
    //     return trace(rhs.expr_lhs())*rhs.expr_rhs();
    //   }
    // }
    return get_default();
  }

  using base::m_lhs;
  using base::m_rhs;
  using base::m_seq_lhs;
  using base::m_seq_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class inner_product_simplifier_otimes final
    : public inner_product_simplifier_default<ExprLHS, ExprRHS> {
public:
  using base = inner_product_simplifier_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_default;
  using value_type = base::value_type;

  inner_product_simplifier_otimes(ExprLHS &&lhs, sequence &&sequence_lhs,
                                  ExprRHS &&rhs, sequence &&sequence_rhs)
      : base(std::forward<ExprLHS>(lhs), std::move(sequence_lhs),
             std::forward<ExprRHS>(rhs), std::move(sequence_rhs)),
        m_lhs_expr(lhs.template get<outer_product_wrapper<value_type>>()) {}

  template <typename Expr> constexpr inline auto operator()(Expr const &rhs) {
    // otimesu(I,I) : expr --> expr
    const detail::otimes_check<value_type, kronecker_delta, kronecker_delta>
        check(sequence{0, 2}, sequence{1, 3});
    if (((m_seq_lhs == sequence{3, 4} && m_seq_rhs == sequence{1, 2}) ||
         (m_seq_lhs == sequence{1, 2} && m_seq_rhs == sequence{1, 2})) &&
        rhs.rank() == 2 && check.inner_check(m_lhs)) {
      return std::forward<ExprRHS>(m_rhs);
    }
    return get_default();
  }

  using base::m_lhs;
  using base::m_rhs;
  using base::m_seq_lhs;
  using base::m_seq_rhs;
  outer_product_wrapper<value_type> const &m_lhs_expr;
};

template <typename ExprLHS, typename ExprRHS> class inner_product_simplifier {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;

  inner_product_simplifier(ExprLHS &&lhs, sequence &&sequence_lhs,
                           ExprRHS &&rhs, sequence &&sequence_rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)),
        m_seq_lhs(std::move(sequence_lhs)), m_seq_rhs(std::move(sequence_rhs)) {
  }

  inner_product_simplifier(ExprLHS &&lhs, sequence const &sequence_lhs,
                           ExprRHS &&rhs, sequence const &sequence_rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)),
        m_seq_lhs(sequence_lhs), m_seq_rhs(sequence_rhs) {}

  // I [*] expr
  constexpr inline auto operator()(kronecker_delta<value_type> const &) {
    return std::visit(
        inner_product_simplifier_kronecker_delta<ExprLHS, ExprRHS>(
            std::forward<ExprLHS>(m_lhs), std::move(m_seq_lhs),
            std::forward<ExprRHS>(m_rhs), std::move(m_seq_rhs)),
        *m_rhs);
  }

  // otimesu(I,I) [*] expr --> outer_product
  constexpr inline auto operator()(outer_product_wrapper<value_type> const &) {
    return std::visit(inner_product_simplifier_otimes<ExprLHS, ExprRHS>(
                          std::forward<ExprLHS>(m_lhs), std::move(m_seq_lhs),
                          std::forward<ExprRHS>(m_rhs), std::move(m_seq_rhs)),
                      *m_rhs);
  }

  // 0.5*(otimeso(I,I) + otimesl(I,I)) [*] expr --> tensor_with_scalar_mul

  template <typename T> constexpr inline auto operator()(T const &) {
    const auto &rhs{*m_rhs};
    inner_product_simplifier_default<ExprLHS, ExprRHS> visitor(
        std::forward<ExprLHS>(m_lhs), std::move(m_seq_lhs),
        std::forward<ExprRHS>(m_rhs), std::move(m_seq_rhs));
    return std::visit(visitor, rhs);
  }

private:
  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
  sequence m_seq_lhs;
  sequence m_seq_rhs;
};

} // namespace numsim::cas
#endif // TENSOR_INNER_PRODUCT_SIMPLIFIER_H
