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
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;

  add_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  [[nodiscard]] auto get_default() {
    if (m_lhs.get().hash_value() == m_rhs.get().hash_value()) {
      auto constant{make_expression<scalar_constant<value_type>>(2)};
      return make_expression<tensor_scalar_mul<value_type>>(
          std::move(constant), std::forward<ExprRHS>(m_rhs));
    }
    // const auto lhs_constant{is_same<tensor_constant<value_type>>(m_lhs)};
    // const auto rhs_constant{is_same<tensor_constant<value_type>>(m_rhs)};
    auto add_new{make_expression<tensor_add<value_type>>(m_lhs.get().dim(),
                                                         m_rhs.get().rank())};
    auto &add{add_new.template get<tensor_add<value_type>>()};
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
  operator()(tensor_negative<value_type> const &expr) {
    if (m_lhs.get().hash_value() == expr.expr().get().hash_value()) {
      return make_expression<tensor_zero<value_type>>(expr.dim(), expr.rank());
    }
    return get_default();
  }

  template <typename Expr>
  [[nodiscard]] constexpr inline expr_type operator()(Expr const &) {
    return get_default();
  }

  [[nodiscard]] constexpr inline expr_type
  operator()(tensor_zero<value_type> const &) {
    return std::forward<ExprLHS>(m_lhs);
  }

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

// template <typename ExprLHS, typename ExprRHS>
// class constant_add final : public add_default<ExprLHS, ExprRHS>{
// public:
//   using value_type = typename std::remove_reference_t<
//       std::remove_const_t<ExprLHS>>::value_type;
//   using expr_type = expression_holder<scalar_expression<value_type>>;
//   using base = add_default<ExprLHS, ExprRHS>;
//   using base::operator();
//   using base::get_coefficient;
//   constant_add(ExprLHS lhs, ExprRHS
//   rhs):base(std::forward<ExprLHS>(lhs),std::forward<ExprRHS>(rhs)),
//                                            lhs(base::m_lhs.template
//                                            get<tensor_constant<value_type>>()){}

//  constexpr inline expr_type operator()(tensor_constant<value_type> const&
//  rhs){
//    const auto value{lhs() + rhs()};
//    return make_expression<tensor_constant<value_type>>(value);
//  }

////  constexpr inline expr_type
/// operator()([[maybe_unused]]tensor_add<value_type> const& rhs){ /    auto
/// add_expr{make_expression<scalar_add<value_type>>(rhs)}; /    auto
///&add{add_expr.template get<scalar_add<value_type>>()}; /    auto
/// coeff{add.coeff() + base::m_lhs}; /    add.set_coeff(std::move(coeff)); /
/// return std::move(add_expr); /  }

// private:
//   tensor_constant<value_type> const& lhs;
// };

template <typename ExprLHS, typename ExprRHS>
class n_ary_add final : public add_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_default;
  // using base::get_coefficient;

  n_ary_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor_add<value_type>>()} {}

  //  constexpr inline expr_type
  //  operator()([[maybe_unused]]tensor_constant<value_type> const& rhs){
  //    auto add_expr{make_expression<tensor_add<value_type>>(lhs)};
  //    auto &add{add_expr.template get<tensor_add<value_type>>()};
  //    auto coeff{add.coeff() + m_rhs};
  //    add.set_coeff(std::move(coeff));
  //    return std::move(add_expr);
  //  }

  //  constexpr inline expr_type
  //  operator()([[maybe_unused]]tensor_one<value_type> const& ){
  //    auto add_expr{make_expression<tensor_add<value_type>>(lhs)};
  //    auto &add{add_expr.template get<tensor_add<value_type>>()};
  //    auto
  //    coeff{make_expression<scalar_constant<value_type>>(get_coefficient(add,
  //    0.0) + static_cast<value_type>(1))};
  //    //auto coeff{add.coeff() + m_rhs};
  //    add.set_coeff(std::move(coeff));
  //    return add_expr;
  //  }

  template <typename Expr> [[nodiscard]] auto operator()(Expr const &rhs) {
    /// do a deep copy of data
    auto expr_add{make_expression<tensor_add<value_type>>(lhs)};
    auto &add{expr_add.template get<tensor_add<value_type>>()};
    /// check if sub_exp == expr_rhs for sub_exp \in expr_lhs
    auto pos{lhs.hash_map().find(rhs.hash_value())};
    if (pos != lhs.hash_map().end()) {
      add.hash_map().erase(rhs.hash_value());
      add.push_back(pos->second + m_rhs);
      return expr_add;
    }
    /// no equal expr or sub_expr
    add.push_back(m_rhs);
    return expr_add;
  }

  // merge two expression
  [[nodiscard]] auto operator()(tensor_add<value_type> const &rhs) {
    auto expr{make_expression<tensor_add<value_type>>(rhs.dim(), rhs.rank())};
    auto &add{expr.template get<tensor_add<value_type>>()};
    merge_add(lhs, rhs, add);
    return expr;
  }

  [[nodiscard]] auto operator()(tensor_negative<value_type> const &rhs) {
    const auto &hash_rhs{rhs.expr().get().hash_value()};
    const auto pos{lhs.hash_map().find(hash_rhs)};
    if (pos != lhs.hash_map().end()) {
      auto expr{make_expression<tensor_add<value_type>>(lhs)};
      auto &add{expr.template get<tensor_add<value_type>>()};
      add.hash_map().erase(hash_rhs);
      return expr;
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_add<value_type> const &lhs;
};

// template<typename T>
// class n_ary_mul_add final : public add_default<T>{
// public:
//   using value_type = T;
//   using expr_type = expression_holder<scalar_expression<value_type>>;
//   using base = add_default<T>;
//   using base::operator();
//   using base::get_default;
//   using base::get_coefficient;

//  n_ary_mul_add(expr_type lhs, expr_type
//  rhs):base(lhs,rhs),lhs{base::m_lhs.template get<scalar_mul<value_type>>()}
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

// private:
//   using base::m_lhs;
//   using base::m_rhs;
//   scalar_mul<value_type> const& lhs;
// };

template <typename ExprLHS, typename ExprRHS>
class symbol_add final : public add_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_default;
  //  using base::get_coefficient;

  symbol_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor<value_type>>()} {}

  /// x+x --> 2*x
  [[nodiscard]] constexpr inline expr_type
  operator()(tensor<value_type> const &rhs) {
    if (&lhs == &rhs) {
      auto new_expr{make_expression<tensor_scalar_mul<value_type>>(
          make_expression<scalar_constant<value_type>>(2), m_rhs)};
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
  //  constexpr inline expr_type operator()(scalar_constant<value_type>
  //  const&rhs) {
  //  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class tensor_scalar_mul_add final : public add_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_default;

  tensor_scalar_mul_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor_scalar_mul<value_type>>()} {}

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
  operator()(tensor_scalar_mul<value_type> const &rhs) {
    if (lhs.expr_rhs().get().hash_value() ==
        rhs.expr_rhs().get().hash_value()) {
      return (lhs.expr_lhs() + rhs.expr_lhs()) * rhs.expr_rhs();
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_scalar_mul<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class add_negative final : public add_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  // using base::get_coefficient;
  using base::get_default;

  add_negative(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor_negative<value_type>>()} {}

  // (-lhs) + (-rhs) --> -(lhs+rhs)
  [[nodiscard]] constexpr inline expr_type
  operator()(tensor_negative<value_type> const &rhs) {
    return -(lhs.expr() + rhs.expr());
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_negative<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS> struct add_base {
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;

  add_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  //  constexpr inline expr_type operator()(scalar_constant<value_type> const&){
  //    return visit(constant_add<ExprLHS, ExprRHS>(m_lhs,m_rhs), *m_rhs);
  //  }

  [[nodiscard]] constexpr inline expr_type
  operator()(tensor_zero<value_type> const &) {
    return std::forward<ExprRHS>(m_rhs);
  }

  [[nodiscard]] constexpr inline expr_type
  operator()(tensor_add<value_type> const &) {
    return visit(n_ary_add<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                             std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  [[nodiscard]] constexpr inline expr_type
  operator()(tensor_scalar_mul<value_type> const &) {
    return visit(
        tensor_scalar_mul_add<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                std::forward<ExprRHS>(m_rhs)),
        *m_rhs);
  }

  [[nodiscard]] constexpr inline expr_type
  operator()(tensor_negative<value_type> const &) {
    return visit(add_negative<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  //  constexpr inline expr_type operator()(scalar_mul<value_type> const&){
  //    return visit(n_ary_mul_add<ExprLHS, ExprRHS>(m_lhs,m_rhs), *m_rhs);
  //  }

  [[nodiscard]] constexpr inline expr_type
  operator()(tensor<value_type> const &) {
    return visit(symbol_add<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                              std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  //  template<typename Expr>
  //  constexpr inline expr_type operator()(Expr const&){
  //    assert(0);
  //    return expr_type{nullptr};
  //  }
  //  constexpr inline expr_type operator()(scalar_zero<value_type> const&){
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
