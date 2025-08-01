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
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;

  sub_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  // rhs is negative
  auto get_default() {
    //    const auto lhs_constant{is_same<tensor_constant<value_type>>(m_lhs)};
    //    const auto rhs_constant{is_same<tensor_constant<value_type>>(m_rhs)};
    auto add_new{make_expression<tensor_add<value_type>>(m_lhs.get().dim(),
                                                         m_lhs.get().rank())};
    auto &add{add_new.template get<tensor_add<value_type>>()};

    //    if(lhs_constant){
    //      add.set_coeff(m_lhs);
    //    }else{
    add.push_back(m_lhs);
    //    }

    //    if(rhs_constant){
    //      add.set_coeff(make_expression<tensor_constant<value_type>>(-m_rhs.template
    //      get<scalar_constant<value_type>>()()));
    //    }else{
    add.push_back(m_rhs);
    //    }
    return std::move(add_new);
  }

  template <typename Expr> constexpr inline expr_type operator()(Expr const &) {
    return get_default();
  }

  //         // expr - 0 --> expr
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

protected:
  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class negative_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::operator();
  // using base::get_coefficient;

  negative_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor_negative<value_type>>()} {}

  //  template<typename Expr>
  //  constexpr inline expr_type
  //  operator()([[maybe_unused]]scalar_add<value_type> const& rhs){
  //    return make_expression<scalar_negative<value_type>>(m_lhs + m_rhs);
  //  }

  //         //-expr - rhs
  //  constexpr inline expr_type operator()(scalar_constant<value_type> const&
  //  rhs){
  //    const auto value{-lhs() - rhs()};
  //    return make_expression<scalar_constant<value_type>>(value);
  //  }

  //  //-expr - (constant + x)
  //  //-expr - constant - x
  //  //-(expr + constant + x)
  //  constexpr inline expr_type
  //  operator()([[maybe_unused]]scalar_add<value_type> const& rhs){
  //    auto add_expr{make_expression<scalar_add<value_type>>(rhs)};
  //    auto &add{add_expr.template get<scalar_add<value_type>>()};
  //    auto coeff{base::m_lhs + add.coeff()};
  //    add.set_coeff(std::move(coeff));
  //    return
  //    make_expression<scalar_negative<value_type>>(std::move(add_expr));
  //  }

  //  //
  //  constexpr inline expr_type operator()(scalar_one<value_type> const& ){
  //    const auto value{lhs() - 1};
  //    return make_expression<scalar_constant<value_type>>(value);
  //  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_negative<value_type> const &lhs;
};

// template<typename T>
// class constant_sub final : public sub_default<T>{
// public:
//   using value_type = T;
//   using expr_type = expression_holder<scalar_expression<value_type>>;
//   using base = sub_default<T>;
//   using base::operator();
//   using base::get_coefficient;

//  constant_sub(expr_type lhs, expr_type
//  rhs):base(lhs,rhs),lhs{base::m_lhs.template
//  get<tensor_constant<value_type>>()}
//  {}

//         //lhs - rhs
//  constexpr inline expr_type operator()(scalar_constant<value_type> const&
//  rhs){
//    const auto value{lhs() - rhs()};
//    return make_expression<scalar_constant<value_type>>(value);
//  }

//         // constant_lhs - (constant + x)
//         // constant_lhs - constant - x
//  constexpr inline expr_type operator()([[maybe_unused]]scalar_add<value_type>
//  const& rhs){
//    assert(true);
//    auto add_expr{make_expression<scalar_add<value_type>>(rhs)};
//    auto &add{add_expr.template get<scalar_add<value_type>>()};
//    auto coeff{base::m_lhs - add.coeff()};
//    add.set_coeff(std::move(coeff));
//    return std::move(add_expr);
//  }

//  constexpr inline expr_type operator()(scalar_one<value_type> const& ){
//    const auto value{lhs() - 1};
//    return make_expression<scalar_constant<value_type>>(value);
//  }

// private:
//   scalar_constant<value_type> const& lhs;
// };

template <typename ExprLHS, typename ExprRHS>
class n_ary_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::operator();
  // using base::get_coefficient;

  n_ary_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor_add<value_type>>()} {}

  //  constexpr inline expr_type
  //  operator()([[maybe_unused]]scalar_constant<value_type> const& rhs){
  //    auto add_expr{make_expression<scalar_add<value_type>>(lhs)};
  //    auto &add{add_expr.template get<scalar_add<value_type>>()};
  //    auto coeff{add.coeff() + m_rhs};
  //    add.set_coeff(std::move(coeff));
  //    return std::move(add_expr);
  //  }

  //  constexpr inline expr_type
  //  operator()([[maybe_unused]]scalar_one<value_type> const& ){
  //    auto add_expr{make_expression<scalar_add<value_type>>(lhs)};
  //    auto &add{add_expr.template get<scalar_add<value_type>>()};
  //    auto
  //    coeff{make_expression<scalar_constant<value_type>>(get_coefficient(add,
  //    0.0) + static_cast<value_type>(1))};
  //    //auto coeff{add.coeff() + m_rhs};
  //    add.set_coeff(std::move(coeff));
  //    return add_expr;
  //  }

  //  auto operator()([[maybe_unused]]scalar<value_type> const&rhs) {
  //    /// do a deep copy of data
  //    auto expr_add{make_expression<scalar_add<value_type>>(lhs)};
  //    auto &add{expr_add.template get<scalar_add<value_type>>()};
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
  //  auto operator()(scalar_add<value_type> const&rhs) {
  //    auto expr{make_expression<scalar_add<value_type>>()};
  //    auto& add{expr.template get<scalar_add<value_type>>()};
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
  tensor_add<value_type> const &lhs;
};

// template<typename T>
// class n_ary_mul_sub final : public sub_default<T>{
// public:
//   using value_type = T;
//   using expr_type = expression_holder<tensor_expression<value_type>>;
//   using base = sub_default<T>;
//   using base::operator();
//   using base::get_default;
//   //using base::get_coefficient;

//  n_ary_mul_sub(expr_type lhs, expr_type
//  rhs):base(lhs,rhs),lhs{base::m_lhs.template get<tensor_mul<value_type>>()}
//  {}

//  //  auto operator()(scalar<value_type> const&rhs) {
//  //    const auto &hash_rhs{rhs.hash_value()};
//  //    const auto &hash_lhs{lhs.hash_value()};
//  //    if (hash_rhs == hash_lhs) {
//  //      auto expr{make_expression<scalar_mul<value_type>>(lhs)};
//  //      auto &mul{expr.template get<scalar_mul<value_type>>()};
//  //      mul.set_coeff(make_expression<scalar_constant<value_type>>(
//  //          get_coefficient(lhs, 1.0) + 1.0));
//  //      return std::move(expr);
//  //    }
//  //    return get_default();
//  //  }

//  //         /// expr + expr --> 2*expr
//  //  auto operator()(scalar_mul<value_type> const&rhs) {
//  //    const auto &hash_rhs{rhs.hash_value()};
//  //    const auto &hash_lhs{lhs.hash_value()};
//  //    if (hash_rhs == hash_lhs) {
//  //      const auto fac_lhs{get_coefficient(lhs, 1.0)};
//  //      const auto fac_rhs{get_coefficient(rhs, 1.0)};
//  //      auto expr{make_expression<scalar_mul<value_type>>(lhs)};
//  //      auto &mul{expr.template get<scalar_mul<value_type>>()};
//  //      mul.set_coeff(
//  //          make_expression<scalar_constant<value_type>>(fac_lhs +
//  fac_rhs));
//  //      return std::move(expr);
//  //    }
//  //    return get_default();
//  //  }

// private:
//   using base::m_lhs;
//   using base::m_rhs;
//   scalar_mul<value_type> const& lhs;
// };

template <typename ExprLHS, typename ExprRHS>
class symbol_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_default;
  // using base::get_coefficient;

  symbol_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor<value_type>>()} {}

  /// x-x --> 0
  constexpr inline expr_type operator()(tensor<value_type> const &rhs) {
    if (&lhs == &rhs) {
      return make_expression<tensor_zero<value_type>>(rhs.dim(), rhs.rank());
    }
    return get_default();
  }

  //         //x - 3*x --> -(2*x)
  //  constexpr inline expr_type  operator()(scalar_mul<value_type> const&rhs) {
  //    const auto &hash_rhs{rhs.hash_value()};
  //    const auto &hash_lhs{lhs.hash_value()};
  //    if (hash_rhs == hash_lhs) {
  //      auto expr{make_expression<scalar_mul<value_type>>(rhs)};
  //      auto &mul{expr.template get<scalar_mul<value_type>>()};
  //      const auto value{1.0 - get_coefficient(rhs, 1.0)};
  //      mul.set_coeff(make_expression<scalar_constant<value_type>>(std::abs(value)));
  //      if(value < 0){
  //        return
  //        make_expression<scalar_negative<value_type>>(std::move(expr));
  //      }else{
  //        return std::move(expr);
  //      }
  //    }
  //    return get_default();
  //  }

  //  constexpr inline expr_type operator()(scalar_constant<value_type>
  //  const&rhs) {
  //  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS> struct sub_base {
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;

  sub_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  //  constexpr inline expr_type operator()(scalar_constant<value_type> const&){
  //    return visit(constant_sub<value_type>(m_lhs,m_rhs), *m_rhs);
  //  }

  constexpr inline expr_type operator()(tensor_add<value_type> const &) {
    return visit(n_ary_sub<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                             std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  //  constexpr inline expr_type operator()(scalar_mul<value_type> const&){
  //    return visit(n_ary_mul_sub<value_type>(m_lhs,m_rhs), *m_rhs);
  //  }

  constexpr inline expr_type operator()(tensor<value_type> const &) {
    return visit(symbol_sub<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                              std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  // 0 - expr
  constexpr inline expr_type operator()(tensor_zero<value_type> const &) {
    return make_expression<tensor_negative<value_type>>(
        std::forward<ExprRHS>(m_rhs));
  }

  // - expr_lhs - expr_rhs --> -(expr_lhs+expr_rhs)
  constexpr inline expr_type
  operator()(tensor_negative<value_type> const &lhs) {
    auto expr{lhs.expr() + std::forward<ExprRHS>(m_rhs)};
    if (expr.is_valid()) {
      return make_expression<tensor_negative<value_type>>(expr);
    }
    return make_expression<tensor_zero<value_type>>(lhs.dim(), lhs.rank());
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
