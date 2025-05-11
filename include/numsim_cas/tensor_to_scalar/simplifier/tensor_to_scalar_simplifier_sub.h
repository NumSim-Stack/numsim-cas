#ifndef TENSOR_SCALAR_SIMPLIFIER_SUB_H
#define TENSOR_SCALAR_SIMPLIFIER_SUB_H

#include "../tensor_to_scalar_expression.h"
#include "../operators/tensor_to_scalar/tensor_to_scalar_sub.h"

namespace numsim::cas {
namespace tensor_to_scalar_detail {
namespace simplifier {

template <typename ExprLHS, typename ExprRHS>
struct sub_default
{
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;

  sub_default(ExprLHS lhs, ExprRHS rhs):m_lhs(lhs),m_rhs(rhs){}

  auto get_default(){
    auto mul_new{make_expression<tensor_to_scalar_sub<value_type>>()};
    auto& mul{mul_new.template get<tensor_to_scalar_sub<value_type>>()};
    mul.push_back(m_lhs);
    mul.push_back(m_rhs);
    return std::move(mul_new);
  }

  template<typename Expr>
  constexpr inline expr_type operator()(Expr const&){
    return get_default();
  }

  ExprLHS m_lhs;
  ExprRHS m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
struct sub_base
{
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;

  sub_base(ExprLHS lhs, ExprRHS rhs):m_lhs(std::forward<ExprLHS>(lhs)),m_rhs(std::forward<ExprRHS>(rhs)){}

  template<typename Type>
  constexpr inline expr_type operator()(Type const&){
    auto& expr_rhs{*m_rhs};
    return visit(sub_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),std::forward<ExprRHS>(m_rhs)), expr_rhs);
  }

  ExprLHS m_lhs;
  ExprRHS m_rhs;
};


}
}
}

#endif // TENSOR_SCALAR_SIMPLIFIER_SUB_H
