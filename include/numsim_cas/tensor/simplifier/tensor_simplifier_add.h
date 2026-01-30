#ifndef TENSOR_SIMPLIFIER_ADD_H
#define TENSOR_SIMPLIFIER_ADD_H

#include "../../expression_holder.h"
#include "../../numsim_cas_forward.h"
#include "../../numsim_cas_type_traits.h"
#include "../../operators.h"
#include "../tensor_functions_fwd.h"
#include "../tensor_std.h"
#include <set>
#include <type_traits>

namespace numsim::cas {
namespace simplifier {
namespace tensor_detail {

template <typename ExprLHS, typename ExprRHS> class add_default {
public:
  using expr_type = expression_holder<tensor_expression>;

  add_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  [[nodiscard]] auto get_default() {
    if (m_lhs.get().hash_value() == m_rhs.get().hash_value()) {
      auto constant{make_expression<scalar_constant>(2)};
      return make_expression<tensor_scalar_mul>(std::move(constant),
                                                std::forward<ExprRHS>(m_rhs));
    }
    // const auto lhs_constant{is_same<tensor_constant>(m_lhs)};
    // const auto rhs_constant{is_same<tensor_constant>(m_rhs)};
    auto add_new{
        make_expression<tensor_add>(m_lhs.get().dim(), m_rhs.get().rank())};
    auto &add{add_new.template get<tensor_add>()};
    // if(lhs_constant){
    //   add.set_coeff(m_lhs);
    // }else{
    add.push_back(m_lhs);
    //}

    // if(rhs_constant){
    //   add.set_coeff(m_rhs);
    // }else{
    add.push_back(m_rhs);
    //}
    return std::move(add_new);
  }

  [[nodiscard]] constexpr inline expr_type
  operator()(tensor_negative const &expr) {
    if (m_lhs.get().hash_value() == expr.expr().get().hash_value()) {
      return make_expression<tensor_zero>(expr.dim(), expr.rank());
    }
    return get_default();
  }

  template <typename Expr>
  [[nodiscard]] constexpr inline expr_type operator()(Expr const &) {
    return get_default();
  }

  [[nodiscard]] constexpr inline expr_type operator()(tensor_zero const &) {
    return std::forward<ExprLHS>(m_lhs);
  }

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

// template <typename ExprLHS, typename ExprRHS>
// class constant_add final : public add_default<ExprLHS, ExprRHS>{
// public:
//   using value_type = typename std::remove_reference_t<
//       std::remove_const_t<ExprLHS>>::value_type;
//   using expr_type = expression_holder<scalar_expression>;
//   using base = add_default<ExprLHS, ExprRHS>;
//   using base::operator();
//   using base::get_coefficient;
//   constant_add(ExprLHS lhs, ExprRHS
//   rhs):base(std::forward<ExprLHS>(lhs),std::forward<ExprRHS>(rhs)),
//                                            lhs(base::m_lhs.template
//                                            get<tensor_constant>()){}

//  constexpr inline expr_type operator()(tensor_constant const&
//  rhs){
//    const auto value{lhs() + rhs()};
//    return make_expression<tensor_constant>(value);
//  }

////  constexpr inline expr_type
/// operator()([[maybe_unused]]tensor_add const& rhs){ /    auto
/// add_expr{make_expression<scalar_add>(rhs)}; /    auto
///&add{add_expr.template get<scalar_add>()}; /    auto
/// coeff{add.coeff() + base::m_lhs}; /    add.set_coeff(std::move(coeff)); /
/// return std::move(add_expr); /  }

// private:
//   tensor_constant const& lhs;
// };

template <typename ExprLHS, typename ExprRHS>
class n_ary_add final : public add_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<tensor_expression>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_default;
  // using base::get_coefficient;

  n_ary_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor_add>()} {}

  //  constexpr inline expr_type
  //  operator()([[maybe_unused]]tensor_constant const& rhs){
  //    auto add_expr{make_expression<tensor_add>(lhs)};
  //    auto &add{add_expr.template get<tensor_add>()};
  //    auto coeff{add.coeff() + m_rhs};
  //    add.set_coeff(std::move(coeff));
  //    return std::move(add_expr);
  //  }

  //  constexpr inline expr_type
  //  operator()([[maybe_unused]]tensor_one const& ){
  //    auto add_expr{make_expression<tensor_add>(lhs)};
  //    auto &add{add_expr.template get<tensor_add>()};
  //    auto
  //    coeff{make_expression<scalar_constant>(get_coefficient(add,
  //    0.0) + static_cast(1))};
  //    //auto coeff{add.coeff() + m_rhs};
  //    add.set_coeff(std::move(coeff));
  //    return add_expr;
  //  }

  template <typename Expr>
  [[nodiscard]] auto operator()([[maybe_unused]] Expr const &rhs) {
    /// do a deep copy of data
    auto expr_add{make_expression<tensor_add>(lhs)};
    auto &add{expr_add.template get<tensor_add>()};
    /// check if sub_exp == expr_rhs for sub_exp \in expr_lhs
    auto pos{lhs.hash_map().find(m_rhs)};
    if (pos != lhs.hash_map().end()) {
      add.hash_map().erase(m_rhs);
      add.push_back(pos->second + m_rhs);
      return expr_add;
    }
    /// no equal expr or sub_expr
    add.push_back(m_rhs);
    return expr_add;
  }

  // merge two expression
  [[nodiscard]] auto operator()(tensor_add const &rhs) {
    auto expr{make_expression<tensor_add>(rhs.dim(), rhs.rank())};
    auto &add{expr.template get<tensor_add>()};
    merge_add(lhs, rhs, add);
    return expr;
  }

  [[nodiscard]] auto operator()(tensor_negative const &rhs) {
    const auto &expr_rhs{rhs.expr()};
    const auto pos{lhs.hash_map().find(expr_rhs)};
    if (pos != lhs.hash_map().end()) {
      auto expr{make_expression<tensor_add>(lhs)};
      auto &add{expr.template get<tensor_add>()};
      add.hash_map().erase(expr_rhs);
      return expr;
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_add const &lhs;
};

// template<typename T>
// class n_ary_mul_add final : public add_default<T>{
// public:
//   using value_type = T;
//   using expr_type = expression_holder<scalar_expression>;
//   using base = add_default<T>;
//   using base::operator();
//   using base::get_default;
//   using base::get_coefficient;

//  n_ary_mul_add(expr_type lhs, expr_type
//  rhs):base(lhs,rhs),lhs{base::m_lhs.template get<scalar_mul>()}
//  {}

//  auto operator()(scalar const&rhs) {
//    const auto &hash_rhs{rhs.hash_value()};
//    const auto &hash_lhs{lhs.hash_value()};
//    if (hash_rhs == hash_lhs) {
//      auto expr{make_expression<scalar_mul>(lhs)};
//      auto &mul{expr.template get<scalar_mul>()};
//      mul.set_coeff(make_expression<scalar_constant>(
//          get_coefficient(lhs, 1.0) + 1.0));
//      return std::move(expr);
//    }
//    return get_default();
//  }

//         /// expr + expr --> 2*expr
//  auto operator()(scalar_mul const&rhs) {
//    const auto &hash_rhs{rhs.hash_value()};
//    const auto &hash_lhs{lhs.hash_value()};
//    if (hash_rhs == hash_lhs) {
//      const auto fac_lhs{get_coefficient(lhs, 1.0)};
//      const auto fac_rhs{get_coefficient(rhs, 1.0)};
//      auto expr{make_expression<scalar_mul>(lhs)};
//      auto &mul{expr.template get<scalar_mul>()};
//      mul.set_coeff(
//          make_expression<scalar_constant>(fac_lhs + fac_rhs));
//      return std::move(expr);
//    }
//    return get_default();
//  }

// private:
//   using base::m_lhs;
//   using base::m_rhs;
//   scalar_mul const& lhs;
// };

template <typename ExprLHS, typename ExprRHS>
class symbol_add final : public add_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<tensor_expression>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_default;
  //  using base::get_coefficient;

  symbol_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor>()} {}

  /// x+x --> 2*x
  [[nodiscard]] constexpr inline expr_type operator()(tensor const &rhs) {
    if (&lhs == &rhs) {
      auto new_expr{make_expression<tensor_scalar_mul>(
          make_expression<scalar_constant>(2), m_rhs)};
      return std::move(new_expr);
    }
    return get_default();
  }

  //  constexpr inline expr_type  operator()(scalar_mul const&rhs) {
  //    const auto &hash_rhs{rhs.hash_value()};
  //    const auto &hash_lhs{lhs.hash_value()};
  //    if (hash_rhs == hash_lhs) {
  //      auto expr{make_expression<scalar_mul>(rhs)};
  //      auto &mul{expr.template get<scalar_mul>()};
  //      mul.set_coeff(make_expression<scalar_constant>(
  //          get_coefficient(rhs, 1.0) + 1.0));
  //      return std::move(expr);
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

template <typename ExprLHS, typename ExprRHS>
class tensor_scalar_mul_add final : public add_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<tensor_expression>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_default;

  tensor_scalar_mul_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor_scalar_mul>()} {}

  // scalar_expr * lhs + rhs
  template <typename Expr>
  [[nodiscard]] constexpr inline expr_type operator()(Expr const &rhs) {
    if (lhs.expr_rhs().get().hash_value() == rhs.hash_value()) {
      return (lhs.expr_lhs() + 1) * m_rhs;
    }
    return get_default();
  }

  // scalar_expr_lhs * lhs + scalar_expr_rhs * rhs
  [[nodiscard]] constexpr inline expr_type
  operator()(tensor_scalar_mul const &rhs) {
    if (lhs.expr_rhs().get().hash_value() ==
        rhs.expr_rhs().get().hash_value()) {
      return (lhs.expr_lhs() + rhs.expr_lhs()) * rhs.expr_rhs();
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_scalar_mul const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class add_negative final : public add_default<ExprLHS, ExprRHS> {
public:
  using expr_type = expression_holder<tensor_expression>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  // using base::get_coefficient;
  using base::get_default;

  add_negative(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor_negative>()} {}

  // (-lhs) + (-rhs) --> -(lhs+rhs)
  [[nodiscard]] constexpr inline expr_type
  operator()(tensor_negative const &rhs) {
    return -(lhs.expr() + rhs.expr());
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_negative const &lhs;
};

template <typename ExprLHS, typename ExprRHS> struct add_base {
  using expr_type = expression_holder<tensor_expression>;

  add_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  //  constexpr inline expr_type operator()(scalar_constant const&){
  //    return visit(constant_add<ExprLHS, ExprRHS>(m_lhs,m_rhs), *m_rhs);
  //  }

  [[nodiscard]] constexpr inline expr_type operator()(tensor_zero const &) {
    return std::forward<ExprRHS>(m_rhs);
  }

  [[nodiscard]] constexpr inline expr_type operator()(tensor_add const &) {
    return visit(n_ary_add<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                             std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  [[nodiscard]] constexpr inline expr_type
  operator()(tensor_scalar_mul const &) {
    return visit(
        tensor_scalar_mul_add<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                std::forward<ExprRHS>(m_rhs)),
        *m_rhs);
  }

  [[nodiscard]] constexpr inline expr_type operator()(tensor_negative const &) {
    return visit(add_negative<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  //  constexpr inline expr_type operator()(scalar_mul const&){
  //    return visit(n_ary_mul_add<ExprLHS, ExprRHS>(m_lhs,m_rhs), *m_rhs);
  //  }

  [[nodiscard]] constexpr inline expr_type operator()(tensor const &) {
    return visit(symbol_add<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                              std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  //  template<typename Expr>
  //  constexpr inline expr_type operator()(Expr const&){
  //    assert(0);
  //    return expr_type{nullptr};
  //  }
  //  constexpr inline expr_type operator()(scalar_zero const&){
  //    return m_rhs;
  //  }

  template <typename Type>
  [[nodiscard]] constexpr inline expr_type operator()(Type const &) {
    return visit(add_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                               std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};
} // namespace tensor_detail
} // namespace simplifier

} // namespace numsim::cas
#endif // TENSOR_SIMPLIFIER_ADD_H
