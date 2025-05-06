#ifndef TENSOR_TO_SCALAR_SIMPLIFIER_ADD_H
#define TENSOR_TO_SCALAR_SIMPLIFIER_ADD_H

#include "../../functions.h"
#include "../tensor_to_scalar_expression.h"
#include "../operators/tensor_to_scalar/tensor_to_scalar_add.h"
#include "../operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_mul.h"

namespace symTM {
namespace tensor_to_scalar_detail {
namespace simplifier {
template <typename ExprLHS, typename ExprRHS>
struct add_default
{
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;

  add_default(ExprLHS lhs, ExprRHS rhs):m_lhs(lhs),m_rhs(rhs){}

  template<typename Expr>
  [[nodiscard]] constexpr inline auto operator()(Expr const&)noexcept{
    return get_default();
  }

  //tensor_to_scalar + tensor_scalar_with_scalar_add --> tensor_scalar_with_scalar_add
  [[nodiscard]] constexpr inline auto operator()(tensor_to_scalar_with_scalar_add<value_type> const& rhs)noexcept{
      return make_expression<tensor_to_scalar_with_scalar_add<value_type>>(rhs.expr_lhs(), rhs.expr_rhs() + m_lhs);
  }

protected:
  [[nodiscard]] constexpr inline auto get_default()noexcept{
    if(m_rhs == m_lhs){
      return get_default_same();
    }
    return get_default_imp();
  }

  [[nodiscard]] constexpr inline auto get_default_same()noexcept{
    auto coef{make_expression<scalar_constant<value_type>>(2)};
    return make_expression<tensor_to_scalar_with_scalar_mul<value_type>>(std::move(coef), m_rhs);
  }

  [[nodiscard]] constexpr inline auto get_default_imp()noexcept{
    auto add_new{make_expression<tensor_to_scalar_add<value_type>>()};
    auto& add{add_new.template get<tensor_to_scalar_add<value_type>>()};
    add.push_back(m_lhs);
    add.push_back(m_rhs);
    return std::move(add_new);
  }

  ExprLHS m_lhs;
  ExprRHS m_rhs;
};


template <typename ExprLHS, typename ExprRHS>
class n_ary_add final : public add_default<ExprLHS, ExprRHS>{
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  //using base::get_coefficient;

  n_ary_add(ExprLHS lhs, ExprRHS rhs):base(std::forward<ExprLHS>(lhs),std::forward<ExprRHS>(rhs)),
                                        lhs{base::m_lhs.template get<tensor_to_scalar_add<value_type>>()}
  {}

  //merge two expression
  auto operator()(tensor_to_scalar_add<value_type> const&rhs) {
    auto expr{make_expression<tensor_to_scalar_add<value_type>>()};
    auto& add{expr.template get<tensor_to_scalar_add<value_type>>()};
    merge_add(lhs, rhs, add);
    return expr;
  }
private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_to_scalar_add<value_type> const& lhs;
};

//tensor_scalar_with_scalar_add + tensor_to_scalar --> tensor_to_scalar_with_scalar_add
//tensor_to_scalar + tensor_to_scalar_with_scalar_add --> tensor_to_scalar_with_scalar_add
//tensor_scalar_with_scalar_add + tensor_scalar_with_scalar_add --> tensor_to_scalar_with_scalar_add
template <typename ExprLHS, typename ExprRHS>
class wrapper_tensor_to_scalar_add_add final : public add_default<ExprLHS, ExprRHS>{
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;
  using base = add_default<ExprLHS, ExprRHS>;

  wrapper_tensor_to_scalar_add_add(ExprLHS lhs, ExprRHS rhs):base(std::forward<ExprLHS>(lhs),std::forward<ExprRHS>(rhs)),
                                                               lhs{base::m_lhs.template get<tensor_to_scalar_with_scalar_add<value_type>>()}
  {}

  //tensor_scalar_with_scalar_add + tensor_scalar_with_scalar_add --> tensor_scalar_with_scalar_add
  constexpr inline expr_type operator()(tensor_to_scalar_with_scalar_add<value_type> const& rhs){
    return make_expression<tensor_to_scalar_with_scalar_add<value_type>>(lhs.expr_lhs() + rhs.expr_lhs(),
                                                                         lhs.expr_rhs() + rhs.expr_rhs());
  }

  //tensor_scalar_with_scalar_add + tensor_to_scalar --> tensor_scalar_with_scalar_add
  template<typename Expr>
  constexpr inline expr_type operator()(Expr const&){
    return make_expression<tensor_to_scalar_with_scalar_add<value_type>>(lhs.expr_lhs(), m_rhs + lhs.expr_rhs());
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_to_scalar_with_scalar_add<value_type> const& lhs;
};

template <typename ExprLHS, typename ExprRHS>
struct add_base
{
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;

  add_base(ExprLHS lhs, ExprRHS rhs):m_lhs(std::forward<ExprLHS>(lhs)),m_rhs(std::forward<ExprRHS>(rhs)){}

  template<typename Type>
  constexpr inline expr_type operator()(Type const&){
    auto& expr_rhs{*m_rhs};
    return visit(add_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),std::forward<ExprRHS>(m_rhs)), expr_rhs);
  }

  constexpr inline expr_type operator()(tensor_to_scalar_add<value_type> const&){
    auto& expr_rhs{*m_rhs};
    return visit(n_ary_add<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),std::forward<ExprRHS>(m_rhs)), expr_rhs);
  }

  //tensor_scalar_with_scalar_add + tensor_scalar --> tensor_scalar_with_scalar_add
  constexpr inline expr_type operator()(tensor_to_scalar_with_scalar_add<value_type> const&){
    auto& expr_rhs{*m_rhs};
    return visit(wrapper_tensor_to_scalar_add_add<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),std::forward<ExprRHS>(m_rhs)), expr_rhs);
  }

  ExprLHS m_lhs;
  ExprRHS m_rhs;
};
}
}
}

#endif // TENSOR_SCALAR_SIMPLIFIER_ADD_H
