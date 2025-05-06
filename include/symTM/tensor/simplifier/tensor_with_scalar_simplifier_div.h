#ifndef TENSOR_WITH_SCALAR_SIMPLIFIER_DIV_H
#define TENSOR_WITH_SCALAR_SIMPLIFIER_DIV_H

#include <type_traits>
#include "../scalar_expression.h"
#include "../tensor_expression.h"

namespace symTM {
namespace tensor_detail {
namespace simplifier {

template <typename ExprLHS, typename ExprRHS>
struct div_default
{
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;

  div_default(ExprLHS lhs, ExprRHS rhs):m_lhs(std::forward<ExprLHS>(lhs)),m_rhs(std::forward<ExprRHS>(rhs)){}

  template<typename Expr>
  constexpr inline expr_type operator()(Expr const&){
    return get_default();
  }

  //expr / 1 --> expr
  constexpr inline expr_type operator()(scalar_one<value_type> const&){
    return std::forward<ExprLHS>(m_lhs);
  }

protected:
  auto get_default(){
    auto div_new{make_expression<tensor_scalar_div<value_type>>(m_lhs, m_rhs)};
    return std::move(div_new);
  }

  ExprLHS m_lhs;
  ExprRHS m_rhs;
};

//template <typename ExprLHS, typename ExprRHS>
//struct scalar_div_simplifier final : public div_default<ExprLHS, ExprRHS>
//{
//  using value_type = typename std::remove_reference_t<
//      std::remove_const_t<ExprLHS>>::value_type;
//  using expr_type = expression_holder<scalar_expression<value_type>>;
//  using base = div_default<ExprLHS, ExprRHS>;
//  using base::operator();

//  scalar_div_simplifier(ExprLHS lhs, ExprRHS rhs):base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),m_expr(m_lhs.template get<scalar_div<value_type>>()){}

//         // (a/b)/(c/d) --> a*d/(b*c)
//  constexpr inline expr_type operator()(scalar_div<value_type> const& rhs){
//    return make_expression<scalar_div<value_type>>(m_expr.expr_lhs()*rhs.expr_rhs(), m_expr.expr_rhs()*rhs.expr_lhs());
//  }

//         // a/b/c --> a/(b*c)
//  template<typename Expr>
//  constexpr inline expr_type operator()(Expr const&){
//    return make_expression<scalar_div<value_type>>(m_expr.expr_lhs(), m_expr.expr_rhs()*m_rhs);
//  }

//private:
//  using base::m_lhs;
//  using base::m_rhs;
//  scalar_div<value_type> const& m_expr;
//};


template <typename ExprLHS, typename ExprRHS>
struct div_base
{
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;

  div_base(ExprLHS lhs, ExprRHS rhs):m_lhs(std::forward<ExprLHS>(lhs)),m_rhs(std::forward<ExprRHS>(rhs)){}

  template<typename Type>
  constexpr inline expr_type operator()(Type const&){
    auto& expr_rhs{*m_rhs};
    return visit(div_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),std::forward<ExprRHS>(m_rhs)), expr_rhs);
  }

  constexpr inline expr_type operator()(scalar_div<value_type> const&){
    auto& expr_rhs{*m_rhs};
    return visit(scalar_div_simplifier<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),std::forward<ExprRHS>(m_rhs)), expr_rhs);
  }

private:
  ExprLHS m_lhs;
  ExprRHS m_rhs;
};

}
}
}


#endif // TENSOR_WITH_SCALAR_SIMPLIFIER_DIV_H
