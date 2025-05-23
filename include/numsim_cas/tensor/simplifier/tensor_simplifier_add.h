#ifndef TENSOR_SIMPLIFIER_ADD_H
#define TENSOR_SIMPLIFIER_ADD_H

#include <type_traits>
#include <set>
#include "../../operators.h"
#include "../../expression_holder.h"
#include "../tensor_std.h"
#include "../../numsim_cas_forward.h"
#include "../../numsim_cas_type_traits.h"

namespace numsim::cas {

template <typename ExprTypeLHS, typename ExprTypeRHS>
constexpr inline auto binary_add_tensor_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs);

namespace simplifier {
namespace tensor_detail {

template <typename ExprLHS, typename ExprRHS>
class add_default {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;

  add_default(ExprLHS lhs, ExprRHS rhs):m_lhs(std::forward<ExprLHS>(lhs)),m_rhs(std::forward<ExprRHS>(rhs)){}

  auto get_default(){
    //const auto lhs_constant{is_same<tensor_constant<value_type>>(m_lhs)};
    //const auto rhs_constant{is_same<tensor_constant<value_type>>(m_rhs)};
    auto add_new{make_expression<tensor_add<value_type>>(m_lhs.get().dim(), m_rhs.get().rank())};
    auto& add{add_new.template get<tensor_add<value_type>>()};
    //if(lhs_constant){
    //  add.set_coeff(m_lhs);
    //}else{
      add.push_back(m_lhs);
    //}

    //if(rhs_constant){
    //  add.set_coeff(m_rhs);
    //}else{
      add.push_back(m_rhs);
    //}
    return std::move(add_new);
  }

  template<typename Expr>
  constexpr inline expr_type operator()(Expr const&){
    return get_default();
  }

//  constexpr inline expr_type operator()(tensor_zero<value_type> const&){
//    return m_lhs;
//  }

//  template <typename _Expr, typename _ValueType>
//  constexpr auto get_coefficient(_Expr const &expr, _ValueType const &value) {
//    if constexpr (is_detected_v<has_coefficient, _Expr>) {
//      auto func{[&](auto const &coeff) {
//        return coeff.is_valid() ? coeff.template get<tensor_constant<value_type>>()()
//                                : value;
//      }};
//      return func(expr.coeff());
//    }
//    return value;
//  }

protected:
  ExprLHS m_lhs;
  ExprRHS m_rhs;
};

//template <typename ExprLHS, typename ExprRHS>
//class constant_add final : public add_default<ExprLHS, ExprRHS>{
//public:
//  using value_type = typename std::remove_reference_t<
//      std::remove_const_t<ExprLHS>>::value_type;
//  using expr_type = expression_holder<scalar_expression<value_type>>;
//  using base = add_default<ExprLHS, ExprRHS>;
//  using base::operator();
//  using base::get_coefficient;
//  constant_add(ExprLHS lhs, ExprRHS rhs):base(std::forward<ExprLHS>(lhs),std::forward<ExprRHS>(rhs)),
//                                           lhs(base::m_lhs.template get<tensor_constant<value_type>>()){}


//  constexpr inline expr_type operator()(tensor_constant<value_type> const& rhs){
//    const auto value{lhs() + rhs()};
//    return make_expression<tensor_constant<value_type>>(value);
//  }

////  constexpr inline expr_type operator()([[maybe_unused]]tensor_add<value_type> const& rhs){
////    auto add_expr{make_expression<scalar_add<value_type>>(rhs)};
////    auto &add{add_expr.template get<scalar_add<value_type>>()};
////    auto coeff{add.coeff() + base::m_lhs};
////    add.set_coeff(std::move(coeff));
////    return std::move(add_expr);
////  }

//private:
//  tensor_constant<value_type> const& lhs;
//};


template <typename ExprLHS, typename ExprRHS>
class n_ary_add final : public add_default<ExprLHS, ExprRHS>{
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  //using base::get_coefficient;

  n_ary_add(ExprLHS lhs, ExprRHS rhs):base(std::forward<ExprLHS>(lhs),std::forward<ExprRHS>(rhs)),lhs{base::m_lhs.template get<tensor_add<value_type>>()}{}


//  constexpr inline expr_type operator()([[maybe_unused]]tensor_constant<value_type> const& rhs){
//    auto add_expr{make_expression<tensor_add<value_type>>(lhs)};
//    auto &add{add_expr.template get<tensor_add<value_type>>()};
//    auto coeff{add.coeff() + m_rhs};
//    add.set_coeff(std::move(coeff));
//    return std::move(add_expr);
//  }

//  constexpr inline expr_type operator()([[maybe_unused]]tensor_one<value_type> const& ){
//    auto add_expr{make_expression<tensor_add<value_type>>(lhs)};
//    auto &add{add_expr.template get<tensor_add<value_type>>()};
//    auto coeff{make_expression<scalar_constant<value_type>>(get_coefficient(add, 0.0) + static_cast<value_type>(1))};
//    //auto coeff{add.coeff() + m_rhs};
//    add.set_coeff(std::move(coeff));
//    return add_expr;
//  }

  auto operator()([[maybe_unused]]tensor<value_type> const&rhs) {
    /// do a deep copy of data
    auto expr_add{make_expression<tensor_add<value_type>>(lhs)};
    auto &add{expr_add.template get<tensor_add<value_type>>()};
    /// check if sub_exp == expr_rhs for sub_exp \in expr_lhs
    auto pos{lhs.hash_map().find(rhs.hash_value())};
    if (pos != lhs.hash_map().end()) {
      auto expr{binary_add_tensor_simplify(pos->second, m_rhs)};
      add.hash_map().erase(rhs.hash_value());
      add.push_back(expr);
      return expr_add;
    }
    /// no equal expr or sub_expr
    add.push_back(m_rhs);
    return expr_add;
  }

         //merge two expression
  auto operator()(tensor_add<value_type> const&rhs) {
    auto expr{make_expression<tensor_add<value_type>>(rhs.dim(),rhs.rank())};
    auto& add{expr.template get<tensor_add<value_type>>()};
//    add.set_coeff(lhs.coeff() + rhs.coeff());
//    expr_set<expr_type> used_expr;
//    for(auto& child : lhs.hash_map() | std::views::values){
//      auto pos{rhs.hash_map().find(child.get().hash_value())};
//      if(pos != rhs.hash_map().end()){
//        used_expr.insert(pos->second);
//        add.push_back(child + pos->second);
//      }else{
//        add.push_back(child);
//      }
//    }
//    if(used_expr.size() != rhs.size()){
//      for(auto& child : rhs.hash_map() | std::views::values){
//        if(!used_expr.count(child)){
//          add.push_back(child);
//        }
//      }
//    }
    merge_add(lhs,rhs,add);
    return expr;
  }
private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_add<value_type> const& lhs;
};


//template<typename T>
//class n_ary_mul_add final : public add_default<T>{
//public:
//  using value_type = T;
//  using expr_type = expression_holder<scalar_expression<value_type>>;
//  using base = add_default<T>;
//  using base::operator();
//  using base::get_default;
//  using base::get_coefficient;

//  n_ary_mul_add(expr_type lhs, expr_type rhs):base(lhs,rhs),lhs{base::m_lhs.template get<scalar_mul<value_type>>()}
//  {}

//  auto operator()(scalar<value_type> const&rhs) {
//    const auto &hash_rhs{rhs.hash_value()};
//    const auto &hash_lhs{lhs.hash_value()};
//    if (hash_rhs == hash_lhs) {
//      auto expr{make_expression<scalar_mul<value_type>>(lhs)};
//      auto &mul{expr.template get<scalar_mul<value_type>>()};
//      mul.set_coeff(make_expression<scalar_constant<value_type>>(
//          get_coefficient(lhs, 1.0) + 1.0));
//      return std::move(expr);
//    }
//    return get_default();
//  }

//         /// expr + expr --> 2*expr
//  auto operator()(scalar_mul<value_type> const&rhs) {
//    const auto &hash_rhs{rhs.hash_value()};
//    const auto &hash_lhs{lhs.hash_value()};
//    if (hash_rhs == hash_lhs) {
//      const auto fac_lhs{get_coefficient(lhs, 1.0)};
//      const auto fac_rhs{get_coefficient(rhs, 1.0)};
//      auto expr{make_expression<scalar_mul<value_type>>(lhs)};
//      auto &mul{expr.template get<scalar_mul<value_type>>()};
//      mul.set_coeff(
//          make_expression<scalar_constant<value_type>>(fac_lhs + fac_rhs));
//      return std::move(expr);
//    }
//    return get_default();
//  }

//private:
//  using base::m_lhs;
//  using base::m_rhs;
//  scalar_mul<value_type> const& lhs;
//};


template <typename ExprLHS, typename ExprRHS>
class symbol_add final : public add_default<ExprLHS, ExprRHS>{
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_default;
//  using base::get_coefficient;


  symbol_add(ExprLHS lhs, ExprRHS rhs):base(std::forward<ExprLHS>(lhs),std::forward<ExprRHS>(rhs)),lhs{base::m_lhs.template get<tensor<value_type>>()}{}


         /// x+x --> 2*x
  constexpr inline expr_type operator()(tensor<value_type> const&rhs) {
    if (&lhs == &rhs) {
      auto new_expr{make_expression<tensor_scalar_mul<value_type>>(
          make_expression<scalar_constant<value_type>>(2),
          m_rhs
          )};
      return std::move(new_expr);
    }
    return get_default();
  }


//  constexpr inline expr_type  operator()(scalar_mul<value_type> const&rhs) {
//    const auto &hash_rhs{rhs.hash_value()};
//    const auto &hash_lhs{lhs.hash_value()};
//    if (hash_rhs == hash_lhs) {
//      auto expr{make_expression<scalar_mul<value_type>>(rhs)};
//      auto &mul{expr.template get<scalar_mul<value_type>>()};
//      mul.set_coeff(make_expression<scalar_constant<value_type>>(
//          get_coefficient(rhs, 1.0) + 1.0));
//      return std::move(expr);
//    }
//    return get_default();
//  }
  //  constexpr inline expr_type operator()(scalar_constant<value_type> const&rhs) {
  //  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor<value_type> const& lhs;
};



template <typename ExprLHS, typename ExprRHS>
struct add_base
{
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;

  add_base(ExprLHS lhs, ExprRHS rhs):m_lhs(std::forward<ExprLHS>(lhs)),m_rhs(std::forward<ExprRHS>(rhs)){}

//  constexpr inline expr_type operator()(scalar_constant<value_type> const&){
//    return visit(constant_add<ExprLHS, ExprRHS>(m_lhs,m_rhs), *m_rhs);
//  }

  constexpr inline expr_type operator()(tensor_add<value_type> const&){
    return visit(n_ary_add<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),std::forward<ExprRHS>(m_rhs)), *m_rhs);
  }

//  constexpr inline expr_type operator()(scalar_mul<value_type> const&){
//    return visit(n_ary_mul_add<ExprLHS, ExprRHS>(m_lhs,m_rhs), *m_rhs);
//  }

  constexpr inline expr_type operator()(tensor<value_type> const&){
    return visit(symbol_add<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),std::forward<ExprRHS>(m_rhs)), *m_rhs);
  }

//  template<typename Expr>
//  constexpr inline expr_type operator()(Expr const&){
//    assert(0);
//    return expr_type{nullptr};
//  }
//  constexpr inline expr_type operator()(scalar_zero<value_type> const&){
//    return m_rhs;
//  }

  template<typename Type>
  constexpr inline expr_type operator()(Type const&){
    return visit(add_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),std::forward<ExprRHS>(m_rhs)), *m_rhs);
  }

  ExprLHS m_lhs;
  ExprRHS m_rhs;
};
}
}

} // namespace numsim::cas
#endif // TENSOR_SIMPLIFIER_ADD_H
