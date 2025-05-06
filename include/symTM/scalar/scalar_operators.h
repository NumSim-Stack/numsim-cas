#ifndef SCALAR_OPERATORS_H
#define SCALAR_OPERATORS_H

#include "../expression_holder.h"
#include "../scalar/scalar_add.h"
#include "../scalar/scalar_div.h"
#include "../scalar/scalar_expression.h"
#include "../scalar/simplifier/scalar_simplifier_add.h"
#include "../scalar/simplifier/scalar_simplifier_sub.h"
#include "../scalar/simplifier/scalar_simplifier_mul.h"
#include "../scalar/simplifier/scalar_simplifier_div.h"
#include <set>
#include <ranges>
#include <iostream>

#define SMYTM_USE_SIMPLIFICATION



namespace symTM {






// template<typename ValueType>
// class scalar_binary_simplifyer_base
//{
// public:
//   scalar_binary_simplifyer_base(){}
//   scalar_binary_simplifyer_base(scalar_binary_simplifyer_base const &) =
//   delete; scalar_binary_simplifyer_base(scalar_binary_simplifyer_base &&) =
//   delete; const scalar_binary_simplifyer_base
//   &operator=(scalar_binary_simplifyer_base const &) = delete;
//   virtual~scalar_binary_simplifyer_base(){}

//  virtual expression_holder<scalar_expression<ValueType>>
//  apply(expression_holder<scalar_expression<ValueType>> && expr) = 0; virtual
//  expression_holder<scalar_expression<ValueType>>
//  apply(expression_holder<scalar_expression<ValueType>> & expr) = 0;
//};

// template<typename ValueType>
// class scalar_binary_simplifyer_constant //final : public
// VisitorScalar_t<ValueType>, public scalar_binary_simplifyer_base<ValueType>
//{
// public:
//   explicit scalar_binary_simplifyer_constant(scalar_constant<ValueType>
//   const& lhs):m_lhs(lhs){}
//   scalar_binary_simplifyer_constant(scalar_binary_simplifyer_constant const
//   &) = delete;
//   scalar_binary_simplifyer_constant(scalar_binary_simplifyer_constant &&) =
//   delete; const scalar_binary_simplifyer_constant
//   &operator=(scalar_binary_simplifyer_constant const &) = delete;

//  expression_holder<scalar_expression<ValueType>>
//  apply(expression_holder<scalar_expression<ValueType>> && expr)override{
////    auto &expr_ = static_cast<VisitableScalar_t<ValueType> &>(expr.get());
////    expr_.accept(*this);
//    return m_result;
//  }

//  expression_holder<scalar_expression<ValueType>>
//  apply(expression_holder<scalar_expression<ValueType>> & expr)override{
////    auto &expr_ = static_cast<VisitableScalar_t<ValueType> &>(expr.get());
////    expr_.accept(*this);
//    return m_result;
//  }

//  void operator()(scalar<ValueType>& visitable)override{
//  }

//  void operator()(scalar_mul<ValueType>& visitable)override{
//  }

//  void operator()(scalar_add<ValueType>& visitable)override{
//  }

//  void operator()(scalar_negative<ValueType>& visitable)override{
//  }

//  void operator()(scalar_div<ValueType> & visitable)override{
//  }

//  void operator()(scalar_constant<ValueType> & visitable)override{
//  }

//  scalar_constant<ValueType> const& m_lhs;
//  expression_holder<scalar_expression<ValueType>> m_result;
//};

// template<typename ValueType>
// class scalar_make_binary_simplifyer //final : public
// VisitorScalar_t<ValueType>
//{
// public:
//   explicit
//   scalar_make_binary_simplifyer(expression_holder<scalar_expression<ValueType>>
//   & expr_lhs):m_expr_lhs(expr_lhs){}
//   scalar_make_binary_simplifyer(scalar_make_binary_simplifyer const &) =
//   delete; scalar_make_binary_simplifyer(scalar_make_binary_simplifyer &&) =
//   delete; const scalar_make_binary_simplifyer
//   &operator=(scalar_make_binary_simplifyer const &) = delete;

//  auto operator()(expression_holder<scalar_expression<ValueType>> & expr){
//  }

//  void operator()(scalar<ValueType>& visitable)override{
//  }

//  void operator()(scalar_mul<ValueType>& visitable)override{
//  }

//  void operator()(scalar_add<ValueType>& visitable)override{
//  }

//  void operator()(scalar_negative<ValueType>& visitable)override{
//  }

//  void operator()(scalar_div<ValueType> & visitable)override{
//  }

//  void operator()(scalar_constant<ValueType> & visitable)override{
//  }

//  expression_holder<scalar_expression<ValueType>> & m_expr_lhs;
//};

// template<typename ValueType>
// constexpr inline auto operator+(scalar_expression<ValueType>& lhs,
// scalar_expression<ValueType>& rhs){
//   auto add{std::make_unique<scalar_add<ValueType>>()};
//   add->push_back(&lhs);
//   add->push_back(&rhs);
//   return expression_holder<scalar_expression<ValueType>>(std::move(add));
// }

// template<typename ValueType>
// constexpr inline auto
// operator+(expression_holder<scalar_expression<ValueType>> &lhs,
//           expression_holder<scalar_expression<ValueType>> &rhs){
//   if(auto add{dynamic_cast<const scalar_add<ValueType>*>(&*lhs)}){
//     static_cast<scalar_add<ValueType>&>(*lhs).push_back(rhs);
//     return lhs;
//   }else{
//     auto add_new = make_expression<scalar_add<ValueType>>();
//     static_cast<scalar_add<ValueType>&>(*add_new).push_back(lhs);
//     static_cast<scalar_add<ValueType>&>(*add_new).push_back(rhs);
//     return add_new;
//   }
// }

// template<typename ValueType>
// constexpr inline auto
// operator+(expression_holder<scalar_expression<ValueType>> &&lhs,
//           expression_holder<scalar_expression<ValueType>> &rhs){
//     static_cast<scalar_add<ValueType>&>(*lhs).push_back(rhs);
//     return lhs;
// }

template <typename Type> class check_type {
public:
  check_type() {}
  inline auto operator()(Type const &) { return true; }
  template <typename T> inline auto operator()(T const &) { return false; }
};

template <typename Type> class contains_scalar {
public:
  using value_type = typename Type::value_type;
  contains_scalar(Type const &expr) : m_expr(expr) {}

  inline auto operator()(scalar<value_type> const &data) {
    return m_expr == data;
  }
  inline auto operator()(scalar_mul<value_type> const &data) {
    return m_expr == data || loop(data);
  }

  inline auto operator()(scalar_add<value_type> const &data) {
    return m_expr == data || loop(data);
  }

  inline auto operator()(scalar_negative<value_type> const &data) {
    return data == m_expr || std::visit(*this, data.expr());
  }

  inline auto operator()(scalar_div<value_type> const &data) {
    return data == m_expr || std::visit(*this, data.expr_lhs()) ||
           std::visit(*this, data.expr_rhs());
  }

  inline auto operator()(scalar_constant<value_type> const &data) {
    return data == m_expr;
  }

private:
  template <typename T> inline auto loop(T const &data) {
    for (const auto &child : data) {
      if (std::visit(*this, child))
        return true;
    }
    return false;
  }

  Type const &m_expr;
};



//template <typename ValueType> class scalar_variable_expr_simplifier_add {
//public:
//  using expr_type = expression_holder<scalar_expression<ValueType>>;
//  scalar_variable_expr_simplifier_add(expr_type &var, expr_type &expr)
//      : m_var(var), m_expr(expr) {}

//  expr_type operator()(scalar_mul<ValueType> &expr) {
//    /// (constant*m_var)+m_var --> (constant+1)*m_var
//    //std::cout<<m_expr<<std::endl;
//    if (expr.size() == 2) {
//      const auto is_constant{
//          std::visit(check_type<scalar_constant<ValueType>>(), *expr[0])};
//      if (is_constant && expr[1] == m_var) {
//        auto add_new{make_expression<scalar_add<ValueType>>()};
//        add_new.template get<scalar_add<ValueType>>().reserve(expr.size());
//        auto &constant{expr[0].template get<scalar_constant<ValueType>>()};
//        add_new.template get<scalar_add<ValueType>>().push_back(make_expression<scalar_constant<ValueType>>(constant() + 1));
//        //insert element except the first one
//        //        for(auto& child : expr){
////          add_new.template get<scalar_add<value_type>>().push_back(child);
////        }
//        return add_new;
//      }
//    }
//    return expr_type();
//  }

//  expr_type operator()(scalar<ValueType> &expr) {
//    if (&expr == &m_var.template get<scalar<ValueType>>()) {
//      auto mul{make_expression<scalar_mul<ValueType>>()};
//      mul.template get<scalar_mul<ValueType>>().push_back(
//          make_expression<scalar_constant<ValueType>>(2));
//      mul.template get<scalar_mul<ValueType>>().push_back(m_var);
//      return mul;
//    }
//    return expr_type();
//  }

//  template <typename T> expr_type operator()(T &) { return expr_type(); }

//private:
//  expr_type &m_var;
//  expr_type &m_expr;
//};

template <typename ExprTypeLHS, typename ExprTypeRHS>
constexpr inline auto binary_scalar_add_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs){
//  auto &_lhs = *lhs;
//  auto &_rhs = *rhs;
//  return std::visit(
//      scalar_simplifier_add<ExprTypeLHS,ExprTypeRHS>(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
//      _lhs, _rhs);
  return visit(simplifier::add_base<ExprTypeLHS, ExprTypeRHS>(lhs,rhs), *lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
constexpr inline auto binary_scalar_sub_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs){
//  auto &_lhs = *lhs;
//  auto &_rhs = *rhs;
//  return std::visit(
//      scalar_simplifier_add<ExprTypeLHS,ExprTypeRHS>(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
//      _lhs, _rhs);
  return visit(simplifier::sub_base<ExprTypeLHS, ExprTypeRHS>(lhs,rhs), *lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
constexpr inline auto binary_scalar_div_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs){
//  using value_type = typename ExprTypeLHS::value_type;
//  return make_expression<scalar_div<value_type>>(
//      std::forward<ExprTypeRHS>(lhs), std::forward<ExprTypeLHS>(rhs));
  auto &_lhs = *lhs;
  return visit(scalar_detail::simplifier::div_base<ExprTypeLHS, ExprTypeRHS>(std::forward<ExprTypeLHS>(lhs),std::forward<ExprTypeRHS>(rhs)), _lhs);
}

template <typename ExprTypeLHS, typename ExprTypeRHS>
constexpr inline auto binary_scalar_mul_simplify(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs){
//  auto &_lhs = *lhs;
//  auto &_rhs = *rhs;
//  return std::visit(
//      scalar_simplifier_mul<ExprTypeLHS,ExprTypeRHS>(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs)),
//      _lhs, _rhs);
  return visit(simplifier::mul_base<ExprTypeLHS, ExprTypeRHS>(lhs,rhs), *lhs);
}


template <typename ValueType>
struct operator_overload<expression_holder<scalar_expression<ValueType>>,
                         expression_holder<scalar_expression<ValueType>>> {
  using expr_type = scalar_expression<ValueType>;

  template <typename ExprTypeLHS, typename ExprTypeRHS>
  static constexpr inline expression_holder<scalar_expression<ValueType>> add(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
    return binary_scalar_add_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }

  template <typename ExprTypeLHS, typename ExprTypeRHS>
  static constexpr inline expression_holder<scalar_expression<ValueType>> sub(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
    //auto negative{-std::forward<ExprTypeRHS>(rhs)};
    return binary_scalar_sub_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }

  template <typename ExprTypeLHS, typename ExprTypeRHS>
  static constexpr inline expression_holder<scalar_expression<ValueType>> div(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
    return binary_scalar_div_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }

  template <typename ExprTypeLHS, typename ExprTypeRHS>
  static constexpr inline expression_holder<scalar_expression<ValueType>> mul(ExprTypeLHS &&lhs, ExprTypeRHS &&rhs) {
    return binary_scalar_mul_simplify(std::forward<ExprTypeLHS>(lhs), std::forward<ExprTypeRHS>(rhs));
  }
};

} // namespace symTM

       //  expr_type operator()(scalar<value_type> &lhs, scalar<value_type> &rhs) {
       //    if (&lhs == &rhs) {
       //      auto mul{make_expression<scalar_mul<value_type>>()};
       //      mul.template get<scalar_mul<value_type>>().push_back(
       //          make_expression<scalar_constant<value_type>>(2));
       //      mul.template get<scalar_mul<value_type>>().push_back(m_rhs);
       //      return mul;
       //    }else{
       //      auto add_new{make_expression<scalar_add<value_type>>()};
       //      add_new.template get<scalar_add<value_type>>().push_back(m_lhs);
       //      add_new.template get<scalar_add<value_type>>().push_back(m_rhs);
       //      return add_new;
       //    }
       //  }

       //  expr_type operator()(scalar_add<value_type> &lhs, scalar<value_type> &rhs)
       //  {
       //    std::vector<expr_type> to_delete, to_insert;
       //    for (auto &child : lhs) {
       //      /// search for the rhs side inside the child expressions
       //      /// (x+y)+x --> (2*x)+y
       //      auto expr{std::visit(scalar_variable_expr_simplifier_add(m_rhs,
       //      child), *child)}; if (expr && (expr != child)) {
       //        to_delete.push_back(child);
       //        to_insert.push_back(std::move(expr));
       //      }
       //    }
       //    for (auto &child : to_delete) {
       //      lhs.erase(std::find(lhs.begin(), lhs.end(), child));
       //    }
       //    for (auto &child : to_insert) {
       //      lhs.push_back(std::move(child));
       //    }
       //    return m_lhs;
       //  }
#endif // SCALAR_OPERATORS_H
