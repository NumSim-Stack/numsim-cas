#ifndef TENSOR_TO_SCALAR_WITH_SCALAR_SIMPLIFIER_MUL_H
#define TENSOR_TO_SCALAR_WITH_SCALAR_SIMPLIFIER_MUL_H


#include "../../functions.h"
#include "../tensor_to_scalar_expression.h"
#include "../operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_mul.h"

namespace symTM {
namespace tensor_to_scalar_with_scalar_detail {
namespace simplifier {

template <typename ExprLHS, typename ExprRHS>
struct mul_default
{
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;

  mul_default(ExprLHS lhs, ExprRHS rhs):m_lhs(lhs),m_rhs(rhs){}

  ExprLHS m_lhs;
  ExprRHS m_rhs;
};


template <typename ExprLHS, typename ExprRHS>
class wrapper_scalar_mul final : public mul_default<ExprLHS, ExprRHS>{
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;
  using base = mul_default<ExprLHS, ExprRHS>;

  wrapper_scalar_mul(ExprLHS lhs, ExprRHS rhs):base(std::forward<ExprLHS>(lhs),std::forward<ExprRHS>(rhs))
  {}

         //scalar * tensor_scalar --> tensor_to_scalar_with_scalar_mul
  constexpr inline expr_type operator()(tensor_to_scalar_expression<value_type> const&){
    return make_expression<tensor_to_scalar_with_scalar_mul<value_type>>(m_lhs, m_rhs);
  }

         //scalar * tensor_to_scalar_with_scalar_mul --> tensor_to_scalar_with_scalar_mul
  constexpr inline expr_type operator()(tensor_to_scalar_with_scalar_mul<value_type> const& rhs){
    return make_expression<tensor_to_scalar_with_scalar_mul<value_type>>(m_lhs * rhs.expr_lhs(), rhs.expr_rhs());
  }

private:
  using base::m_lhs;
  using base::m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class wrapper_tensor_to_scalar_mul final : public mul_default<ExprLHS, ExprRHS>{
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;
  using base = mul_default<ExprLHS, ExprRHS>;

  wrapper_tensor_to_scalar_mul(ExprLHS lhs, ExprRHS rhs):base(std::forward<ExprLHS>(lhs),std::forward<ExprRHS>(rhs))
  {}

         //tensor_scalar * scalar --> tensor_to_scalar_with_scalar_mul
  constexpr inline expr_type operator()(scalar_expression<value_type> const&){
    //{scalar, tensor_to_scalar}
    return make_expression<tensor_to_scalar_with_scalar_mul<value_type>>(m_rhs, m_lhs);
  }

private:
  using base::m_lhs;
  using base::m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class wrapper_tensor_to_scalar_mul_mul final : public mul_default<ExprLHS, ExprRHS>{
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;
  using base = mul_default<ExprLHS, ExprRHS>;

  wrapper_tensor_to_scalar_mul_mul(ExprLHS lhs, ExprRHS rhs):base(std::forward<ExprLHS>(lhs),std::forward<ExprRHS>(rhs)),
                                                               lhs{base::m_lhs.template get<tensor_to_scalar_with_scalar_mul<value_type>>()}
  {}

         //tensor_to_scalar_with_scalar_mul * scalar --> tensor_scalar_with_scalar_add
  constexpr inline expr_type operator()(scalar_expression<value_type> const&){
    //{scalar, tensor_to_scalar}
    return make_expression<tensor_to_scalar_with_scalar_mul<value_type>>(lhs.expr_lhs() * m_rhs, lhs.expr_rhs());
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_to_scalar_with_scalar_mul<value_type> const& lhs;
};


//scalar * tensor_to_scalar --> tensor_to_scalar_with_scalar_add
//scalar * tensor_to_scalar_with_scalar_add --> tensor_to_scalar_with_scalar_add
//tensor_to_scalar * scalar --> tensor_scalar_with_scalar_add
//tensor_to_scalar_with_scalar_add * scalar --> tensor_to_scalar_with_scalar_add
template <typename ExprLHS, typename ExprRHS>
struct mul_base
{
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;

  mul_base(ExprLHS lhs, ExprRHS rhs):m_lhs(std::forward<ExprLHS>(lhs)),m_rhs(std::forward<ExprRHS>(rhs)){}

  template<typename Expr,
            std::enable_if_t<std::is_base_of_v<tensor_to_scalar_expression<value_type>,Expr>,bool> = true>
  constexpr inline expr_type operator()(Expr const&){
    auto& expr_rhs{*m_rhs};
    return visit(wrapper_tensor_to_scalar_mul<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),std::forward<ExprRHS>(m_rhs)), expr_rhs);
  }

  template<typename Expr, std::enable_if_t<std::is_base_of_v<scalar_expression<value_type>,Expr>,bool> = true>
  constexpr inline expr_type operator()(Expr const&){
    auto& expr_rhs{*m_rhs};
    return visit(wrapper_scalar_mul<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),std::forward<ExprRHS>(m_rhs)), expr_rhs);
  }

  constexpr inline expr_type operator()(tensor_to_scalar_with_scalar_mul<value_type> const&){
    auto& expr_rhs{*m_rhs};
    return visit(wrapper_tensor_to_scalar_mul_mul<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),std::forward<ExprRHS>(m_rhs)), expr_rhs);
  }

  ExprLHS m_lhs;
  ExprRHS m_rhs;
};
}
}
}

#endif // TENSOR_TO_SCALAR_WITH_SCALAR_SIMPLIFIER_MUL_H
