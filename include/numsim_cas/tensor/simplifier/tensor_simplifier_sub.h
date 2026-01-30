#ifndef TENSOR_SIMPLIFIER_SUB_H
#define TENSOR_SIMPLIFIER_SUB_H

#include "../../expression_holder.h"
#include "../../numsim_cas_forward.h"
#include "../../numsim_cas_type_traits.h"
#include "../../operators.h"
#include "../tensor_std.h"
#include <set>
#include <type_traits>

namespace numsim::cas {
template <typename ExprTypeLHS, typename ExprTypeRHS>
constexpr inline auto binary_tensor_sub_simplify(ExprTypeLHS &&lhs,
                                                 ExprTypeRHS &&rhs);
namespace tensor_detail {
namespace simplifier {
template <typename ExprLHS, typename ExprRHS> class sub_default {
public:
  using expr_type = expression_holder<tensor_expression>;

  sub_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  // rhs is negative
  auto get_default() {
    //    const auto lhs_constant{is_same<tensor_constant>(m_lhs)};
    //    const auto rhs_constant{is_same<tensor_constant>(m_rhs)};
    auto add_new{
        make_expression<tensor_add>(m_lhs.get().dim(), m_lhs.get().rank())};
    auto &add{add_new.template get<tensor_add>()};

    //    if(lhs_constant){
    //      add.set_coeff(m_lhs);
    //    }else{
    add.push_back(m_lhs);
    //    }

    //    if(rhs_constant){
    //      add.set_coeff(make_expression<tensor_constant>(-m_rhs.template
    //      get<scalar_constant>()()));
    //    }else{
    add.push_back(m_rhs);
    //    }
    return std::move(add_new);
  }

  template <typename Expr> constexpr inline expr_type operator()(Expr const &) {
    return get_default();
  }

  //         // expr - 0 --> expr
  //  constexpr inline expr_type operator()(tensor_zero const&){
  //    return m_lhs;
  //  }

  //  template <typename _Expr, typename _ValueType>
  //  constexpr auto get_coefficient(_Expr const &expr, _ValueType const &value)
  //  {
  //    if constexpr (is_detected_v<has_coefficient, _Expr>) {
  //      auto func{[&](auto const &coeff) {
  //        return coeff.is_valid() ? coeff.template
  //        get<tensor_constant>()()
  //                                : value;
  //      }};
  //      return func(expr.coeff());
  //    }
  //    return value;
  //  }

protected:
  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class negative_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<tensor_expression>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::operator();
  // using base::get_coefficient;

  negative_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor_negative>()} {}

  //  template<typename Expr>
  //  constexpr inline expr_type
  //  operator()([[maybe_unused]]scalar_add const& rhs){
  //    return make_expression<scalar_negative>(m_lhs + m_rhs);
  //  }

  //         //-expr - rhs
  //  constexpr inline expr_type operator()(scalar_constant const&
  //  rhs){
  //    const auto value{-lhs() - rhs()};
  //    return make_expression<scalar_constant>(value);
  //  }

  //  //-expr - (constant + x)
  //  //-expr - constant - x
  //  //-(expr + constant + x)
  //  constexpr inline expr_type
  //  operator()([[maybe_unused]]scalar_add const& rhs){
  //    auto add_expr{make_expression<scalar_add>(rhs)};
  //    auto &add{add_expr.template get<scalar_add>()};
  //    auto coeff{base::m_lhs + add.coeff()};
  //    add.set_coeff(std::move(coeff));
  //    return
  //    make_expression<scalar_negative>(std::move(add_expr));
  //  }

  //  //
  //  constexpr inline expr_type operator()(scalar_one const& ){
  //    const auto value{lhs() - 1};
  //    return make_expression<scalar_constant>(value);
  //  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_negative const &lhs;
};

// template<typename T>
// class constant_sub final : public sub_default<T>{
// public:
//   using value_type = T;
//   using expr_type = expression_holder<scalar_expression>;
//   using base = sub_default<T>;
//   using base::operator();
//   using base::get_coefficient;

//  constant_sub(expr_type lhs, expr_type
//  rhs):base(lhs,rhs),lhs{base::m_lhs.template
//  get<tensor_constant>()}
//  {}

//         //lhs - rhs
//  constexpr inline expr_type operator()(scalar_constant const&
//  rhs){
//    const auto value{lhs() - rhs()};
//    return make_expression<scalar_constant>(value);
//  }

//         // constant_lhs - (constant + x)
//         // constant_lhs - constant - x
//  constexpr inline expr_type operator()([[maybe_unused]]scalar_add
//  const& rhs){
//    assert(true);
//    auto add_expr{make_expression<scalar_add>(rhs)};
//    auto &add{add_expr.template get<scalar_add>()};
//    auto coeff{base::m_lhs - add.coeff()};
//    add.set_coeff(std::move(coeff));
//    return std::move(add_expr);
//  }

//  constexpr inline expr_type operator()(scalar_one const& ){
//    const auto value{lhs() - 1};
//    return make_expression<scalar_constant>(value);
//  }

// private:
//   scalar_constant const& lhs;
// };

template <typename ExprLHS, typename ExprRHS>
class n_ary_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<tensor_expression>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::operator();
  // using base::get_coefficient;

  n_ary_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor_add>()} {}

  //  constexpr inline expr_type
  //  operator()([[maybe_unused]]scalar_constant const& rhs){
  //    auto add_expr{make_expression<scalar_add>(lhs)};
  //    auto &add{add_expr.template get<scalar_add>()};
  //    auto coeff{add.coeff() + m_rhs};
  //    add.set_coeff(std::move(coeff));
  //    return std::move(add_expr);
  //  }

  //  constexpr inline expr_type
  //  operator()([[maybe_unused]]scalar_one const& ){
  //    auto add_expr{make_expression<scalar_add>(lhs)};
  //    auto &add{add_expr.template get<scalar_add>()};
  //    auto
  //    coeff{make_expression<scalar_constant>(get_coefficient(add,
  //    0.0) + static_cast(1))};
  //    //auto coeff{add.coeff() + m_rhs};
  //    add.set_coeff(std::move(coeff));
  //    return add_expr;
  //  }

  //  auto operator()([[maybe_unused]]scalar const&rhs) {
  //    /// do a deep copy of data
  //    auto expr_add{make_expression<scalar_add>(lhs)};
  //    auto &add{expr_add.template get<scalar_add>()};
  //    /// check if sub_exp == expr_rhs for sub_exp \in expr_lhs
  //    auto pos{lhs.hash_map().find(rhs.hash_value())};
  //    if (pos != lhs.hash_map().end()) {
  //      auto expr{binary_scalar_add_simplify(pos->second, m_rhs)};
  //      add.hash_map().erase(rhs.hash_value());
  //      add.push_back(expr);
  //      return expr_add;
  //    }
  //    /// no equal expr or sub_expr
  //    add.push_back(m_rhs);
  //    return expr_add;
  //  }

  //         //merge two expression
  //  auto operator()(scalar_add const&rhs) {
  //    auto expr{make_expression<scalar_add>()};
  //    auto& add{expr.template get<scalar_add>()};
  //    add.set_coeff(lhs.coeff() + rhs.coeff());
  //    std::set<expr_type, expression_comparator> used_expr;
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
  //    return expr;
  //  }
private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_add const &lhs;
};

// template<typename T>
// class n_ary_mul_sub final : public sub_default<T>{
// public:
//   using value_type = T;
//   using expr_type = expression_holder<tensor_expression>;
//   using base = sub_default<T>;
//   using base::operator();
//   using base::get_default;
//   //using base::get_coefficient;

//  n_ary_mul_sub(expr_type lhs, expr_type
//  rhs):base(lhs,rhs),lhs{base::m_lhs.template get<tensor_mul>()}
//  {}

//  //  auto operator()(scalar const&rhs) {
//  //    const auto &hash_rhs{rhs.hash_value()};
//  //    const auto &hash_lhs{lhs.hash_value()};
//  //    if (hash_rhs == hash_lhs) {
//  //      auto expr{make_expression<scalar_mul>(lhs)};
//  //      auto &mul{expr.template get<scalar_mul>()};
//  //      mul.set_coeff(make_expression<scalar_constant>(
//  //          get_coefficient(lhs, 1.0) + 1.0));
//  //      return std::move(expr);
//  //    }
//  //    return get_default();
//  //  }

//  //         /// expr + expr --> 2*expr
//  //  auto operator()(scalar_mul const&rhs) {
//  //    const auto &hash_rhs{rhs.hash_value()};
//  //    const auto &hash_lhs{lhs.hash_value()};
//  //    if (hash_rhs == hash_lhs) {
//  //      const auto fac_lhs{get_coefficient(lhs, 1.0)};
//  //      const auto fac_rhs{get_coefficient(rhs, 1.0)};
//  //      auto expr{make_expression<scalar_mul>(lhs)};
//  //      auto &mul{expr.template get<scalar_mul>()};
//  //      mul.set_coeff(
//  //          make_expression<scalar_constant>(fac_lhs +
//  fac_rhs));
//  //      return std::move(expr);
//  //    }
//  //    return get_default();
//  //  }

// private:
//   using base::m_lhs;
//   using base::m_rhs;
//   scalar_mul const& lhs;
// };

template <typename ExprLHS, typename ExprRHS>
class symbol_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<tensor_expression>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_default;
  // using base::get_coefficient;

  symbol_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor>()} {}

  /// x-x --> 0
  constexpr inline expr_type operator()(tensor const &rhs) {
    if (&lhs == &rhs) {
      return make_expression<tensor_zero>(rhs.dim(), rhs.rank());
    }
    return get_default();
  }

  //         //x - 3*x --> -(2*x)
  //  constexpr inline expr_type  operator()(scalar_mul const&rhs) {
  //    const auto &hash_rhs{rhs.hash_value()};
  //    const auto &hash_lhs{lhs.hash_value()};
  //    if (hash_rhs == hash_lhs) {
  //      auto expr{make_expression<scalar_mul>(rhs)};
  //      auto &mul{expr.template get<scalar_mul>()};
  //      const auto value{1.0 - get_coefficient(rhs, 1.0)};
  //      mul.set_coeff(make_expression<scalar_constant>(std::abs(value)));
  //      if(value < 0){
  //        return
  //        make_expression<scalar_negative>(std::move(expr));
  //      }else{
  //        return std::move(expr);
  //      }
  //    }
  //    return get_default();
  //  }

  //  constexpr inline expr_type operator()(scalar_constant
  //  const&rhs) {
  //  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor const &lhs;
};

template <typename ExprLHS, typename ExprRHS> struct sub_base {
  using expr_type = expression_holder<tensor_expression>;

  sub_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  //  constexpr inline expr_type operator()(scalar_constant const&){
  //    return visit(constant_sub(m_lhs,m_rhs), *m_rhs);
  //  }

  constexpr inline expr_type operator()(tensor_add const &) {
    return visit(n_ary_sub<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                             std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  //  constexpr inline expr_type operator()(scalar_mul const&){
  //    return visit(n_ary_mul_sub(m_lhs,m_rhs), *m_rhs);
  //  }

  constexpr inline expr_type operator()(tensor const &) {
    return visit(symbol_sub<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                              std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  // 0 - expr
  constexpr inline expr_type operator()(tensor_zero const &) {
    return make_expression<tensor_negative>(std::forward<ExprRHS>(m_rhs));
  }

  // - expr_lhs - expr_rhs --> -(expr_lhs+expr_rhs)
  constexpr inline expr_type operator()(tensor_negative const &lhs) {
    auto expr{lhs.expr() + std::forward<ExprRHS>(m_rhs)};
    if (expr.is_valid()) {
      return make_expression<tensor_negative>(expr);
    }
    return make_expression<tensor_zero>(lhs.dim(), lhs.rank());
  }

  template <typename Type> constexpr inline expr_type operator()(Type const &) {
    return visit(sub_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                               std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};
} // namespace simplifier
} // namespace tensor_detail
} // namespace numsim::cas

#endif // TENSOR_SIMPLIFIER_SUB_H
