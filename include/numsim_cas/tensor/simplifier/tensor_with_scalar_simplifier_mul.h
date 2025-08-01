#ifndef TENSOR_WITH_SCALAR_SIMPLIFIER_MUL_H
#define TENSOR_WITH_SCALAR_SIMPLIFIER_MUL_H

#include "../../expression_holder.h"
#include "../../numsim_cas_forward.h"
#include "../../numsim_cas_type_traits.h"
#include "../../operators.h"
#include "../tensor_std.h"
#include <set>
#include <type_traits>

namespace numsim::cas {
namespace tensor_with_scalar_detail {
namespace simplifier {

template <typename ExprLHS, typename ExprRHS> class mul_default {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_lhs =
      typename std::remove_reference_t<std::remove_const_t<ExprLHS>>;
  using expr_rhs =
      typename std::remove_reference_t<std::remove_const_t<ExprRHS>>;
  using expr_type = expression_holder<tensor_expression<value_type>>;

  mul_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  //  auto get_default(){
  //    //const auto lhs_constant{is_same<tensor_constant<value_type>>(m_lhs)};
  //    //const auto rhs_constant{is_same<tensor_constant<value_type>>(m_rhs)};
  //    auto add_new{make_expression<tensor_add<value_type>>()};
  //    auto& add{add_new.template get<tensor_add<value_type>>()};
  //    //if(lhs_constant){
  //    //  add.set_coeff(m_lhs);
  //    //}else{
  //      add.push_back(m_lhs);
  //    //}

  //    //if(rhs_constant){
  //    //  add.set_coeff(m_rhs);
  //    //}else{
  //      add.push_back(m_rhs);
  //    //}
  //    return std::move(add_new);
  //  }

  //  template<typename Expr>
  //  constexpr inline expr_type operator()(Expr const&){
  //    return get_default();
  //  }

  //  constexpr inline expr_type operator()(tensor_zero<value_type> const&){
  //    return m_lhs;
  //  }

  //  template <typename _Expr, typename _ValueType>
  //  constexpr auto get_coefficient(_Expr const &expr, _ValueType const &value)
  //  {
  //    if constexpr (is_detected_v<has_coefficient, _Expr>) {
  //      auto func{[&](auto const &coeff) {
  //        return coeff.is_valid() ? coeff.template
  //        get<tensor_constant<value_type>>()()
  //                                : value;
  //      }};
  //      return func(expr.coeff());
  //    }
  //    return value;
  //  }

  template <typename Expr> constexpr inline expr_type operator()(Expr const &) {
    static_assert(true, "not supported");
    return expr_type{};
  }

protected:
  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

// template <typename ExprLHS, typename ExprRHS>
// class tensor_scalar_mul_simplifier final : public mul_default<ExprLHS,
// ExprRHS>{ public:
//   using value_type = typename std::remove_reference_t<
//       std::remove_const_t<ExprLHS>>::value_type;
//   using expr_lhs = typename std::remove_reference_t<
//       std::remove_const_t<ExprLHS>>;
//   using expr_rhs = typename std::remove_reference_t<
//       std::remove_const_t<ExprRHS>>;
//   using expr_type = expression_holder<tensor_expression<value_type>>;
//   using base = mul_default<ExprLHS, ExprRHS>;
//   //using base::operator();
//   //using base::get_default;
//   //using base::get_coefficient;

//  tensor_scalar_mul_simplifier(expr_lhs lhs, expr_rhs
//  rhs):base(lhs,rhs),lhs{base::m_lhs.template
//  get<tensor_scalar_mul<value_type>>()}
//  {}

//  constexpr inline expr_type operator()(scalar_expression<value_type> const& )
//  {
//    std::cout<<"constexpr inline expr_type
//    operator()(scalar_expression<value_type> const& ) {"<<std::endl;
//    //auto scalar_expr = lhs.expr_lhs() * m_rhs;
//    return make_expression<tensor_scalar_mul<value_type>>(lhs.expr_lhs() *
//    m_rhs, lhs.expr_rhs());
//    //return expr_type{};
//  }

//  template<typename Expr>
//  constexpr inline expr_type operator()(Expr const&){
//    return make_expression<tensor_scalar_mul<value_type>>(m_rhs, m_lhs);
//  }
// private:
//  using base::m_lhs;
//  using base::m_rhs;
//  tensor_scalar_mul<value_type> const& lhs;
//};

template <typename ExprLHS, typename ExprRHS>
class scalar_tensor_mul final : public mul_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_lhs =
      typename std::remove_reference_t<std::remove_const_t<ExprLHS>>;
  using expr_rhs =
      typename std::remove_reference_t<std::remove_const_t<ExprRHS>>;
  using expr_type = expression_holder<tensor_expression<value_type>>;
  using base = mul_default<ExprLHS, ExprRHS>;
  // using base::operator();
  // using base::get_default;
  // using base::get_coefficient;

  scalar_tensor_mul(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_expression<value_type>>()} {}

  constexpr inline expr_type
  operator()(tensor_scalar_mul<value_type> const &rhs) {
    return make_expression<tensor_scalar_mul<value_type>>(
        rhs.expr_lhs() * m_lhs, rhs.expr_rhs());
  }

  template <typename Expr> constexpr inline expr_type operator()(Expr const &) {
    return make_expression<tensor_scalar_mul<value_type>>(m_lhs, m_rhs);
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_expression<value_type> const &lhs;
};

// lhs := scalar_expression
// rhs := tensor_expression
template <typename ExprLHS, typename ExprRHS> struct mul_base {
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_lhs =
      typename std::remove_reference_t<std::remove_const_t<ExprLHS>>;
  using expr_rhs =
      typename std::remove_reference_t<std::remove_const_t<ExprRHS>>;
  using expr_type = expression_holder<tensor_expression<value_type>>;

  mul_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  //  constexpr inline expr_type operator()(tensor_scalar_mul<value_type>
  //  const&){
  //    return visit(tensor_scalar_mul_simplifier<expr_lhs,
  //    expr_rhs>(m_lhs,m_rhs), *m_rhs);
  //  }

  //  constexpr inline expr_type operator()(scalar_expression<value_type>
  //  const&){
  //    return visit(scalar_tensor_mul_simplifier<expr_lhs,
  //    expr_rhs>(m_lhs,m_rhs), *m_rhs);
  //  }

  template <
      typename Expr,
      std::enable_if_t<std::is_base_of_v<scalar_expression<value_type>, Expr>,
                       bool> = true>
  constexpr inline expr_type operator()(Expr const &) {
    return visit(
        scalar_tensor_mul<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                            std::forward<ExprRHS>(m_rhs)),
        *m_rhs);
  }

  template <
      typename Expr,
      std::enable_if_t<!std::is_base_of_v<scalar_expression<value_type>, Expr>,
                       bool> = true>
  constexpr inline expr_type operator()(Expr const &) {
    static_assert(true, "not supported");
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

} // namespace simplifier
} // namespace tensor_with_scalar_detail
} // namespace numsim::cas

#endif // TENSOR_WITH_SCALAR_SIMPLIFIER_MUL_H
