#ifndef SCALAR_SIMPLIFIER_SUB_H
#define SCALAR_SIMPLIFIER_SUB_H

#include "../../expression_holder.h"
#include "../scalar_functions_fwd.h"
#include "../scalar_globals.h"
#include <set>
#include <type_traits>

namespace numsim::cas {
namespace simplifier {
template <typename ExprLHS, typename ExprRHS> class sub_default {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;

  sub_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  // rhs is negative
  auto get_default() {
    const auto lhs_constant{is_same<scalar_constant<value_type>>(m_lhs) ||
                            is_same<scalar_one<value_type>>(m_lhs)};
    const auto rhs_constant{is_same<scalar_constant<value_type>>(m_rhs) ||
                            is_same<scalar_one<value_type>>(m_rhs)};
    auto add_new{make_expression<scalar_add<value_type>>()};
    auto &add{add_new.template get<scalar_add<value_type>>()};
    if (lhs_constant) {
      add.set_coeff(m_lhs);
    } else {
      add_new.template get<scalar_add<value_type>>().push_back(m_lhs);
    }

    if (rhs_constant) {
      add.set_coeff(make_expression<scalar_constant<value_type>>(
          -m_rhs.template get<scalar_constant<value_type>>()()));
    } else {
      add_new.template get<scalar_add<value_type>>().push_back(-m_rhs);
    }
    return std::move(add_new);
  }

  template <typename Expr> constexpr inline expr_type operator()(Expr const &) {
    return get_default();
  }

  // expr - 0 --> expr
  constexpr inline expr_type operator()(scalar_zero<value_type> const &) {
    return m_lhs;
  }

  template <typename _Expr, typename _ValueType>
  constexpr auto get_coefficient(_Expr const &expr, _ValueType const &value) {
    if constexpr (is_detected_v<has_coefficient, _Expr>) {
      auto func{[&](auto const &coeff) {
        return coeff.is_valid()
                   ? coeff.template get<scalar_constant<value_type>>()()
                   : value;
      }};
      return func(expr.coeff());
    }
    return value;
  }

protected:
  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class negative_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_coefficient;

  negative_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_negative<value_type>>()} {}

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
  scalar_negative<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class constant_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_coefficient;

  constant_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_constant<value_type>>()} {}

  // lhs - rhs
  constexpr inline expr_type
  operator()(scalar_constant<value_type> const &rhs) {
    const auto value{lhs() - rhs()};
    return make_expression<scalar_constant<value_type>>(value);
  }

  // constant_lhs - (constant + x)
  // constant_lhs - constant - x
  constexpr inline expr_type
  operator()([[maybe_unused]] scalar_add<value_type> const &rhs) {
    assert(true);
    auto add_expr{make_expression<scalar_add<value_type>>(rhs)};
    auto &add{add_expr.template get<scalar_add<value_type>>()};
    auto coeff{base::m_lhs - add.coeff()};
    add.set_coeff(std::move(coeff));
    return std::move(add_expr);
  }

  constexpr inline expr_type operator()(scalar_one<value_type> const &) {
    const auto value{lhs() - 1};
    return make_expression<scalar_constant<value_type>>(value);
  }

private:
  scalar_constant<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class n_ary_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_coefficient;

  n_ary_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_add<value_type>>()} {}

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
  scalar_add<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class n_ary_mul_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_coefficient;
  using base::get_default;

  n_ary_mul_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_mul<value_type>>()} {}

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

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_mul<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class symbol_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_coefficient;
  using base::get_default;

  symbol_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar<value_type>>()} {}

  /// x-x --> 0
  constexpr inline expr_type operator()(scalar<value_type> const &rhs) {
    if (&lhs == &rhs) {
      return get_scalar_zero<value_type>();
    }
    return get_default();
  }

  // x - 3*x --> -(2*x)
  constexpr inline expr_type operator()(scalar_mul<value_type> const &rhs) {
    const auto &hash_rhs{rhs.hash_value()};
    const auto &hash_lhs{lhs.hash_value()};
    if (hash_rhs == hash_lhs) {
      auto expr{make_expression<scalar_mul<value_type>>(rhs)};
      auto &mul{expr.template get<scalar_mul<value_type>>()};
      const auto value{1.0 - get_coefficient(rhs, 1.0)};
      mul.set_coeff(
          make_expression<scalar_constant<value_type>>(std::abs(value)));
      if (value < 0) {
        return make_expression<scalar_negative<value_type>>(std::move(expr));
      } else {
        return std::move(expr);
      }
    }
    return get_default();
  }

  //  constexpr inline expr_type operator()(scalar_constant<value_type>
  //  const&rhs) {
  //  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class scalar_one_sub final : public sub_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = sub_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_coefficient;
  using base::get_default;

  scalar_one_sub(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_one<value_type>>()} {}

  constexpr inline expr_type
  operator()(scalar_constant<value_type> const &rhs) {
    return make_expression<scalar_constant<value_type>>(
        static_cast<value_type>(1) - rhs());
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_one<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS> struct sub_base {
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;

  sub_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  constexpr inline expr_type operator()(scalar_constant<value_type> const &) {
    return visit(constant_sub<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  constexpr inline expr_type operator()(scalar_add<value_type> const &) {
    return visit(n_ary_sub<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                             std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  constexpr inline expr_type operator()(scalar_mul<value_type> const &) {
    return visit(n_ary_mul_sub<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                 std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  constexpr inline expr_type operator()(scalar<value_type> const &) {
    return visit(symbol_sub<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                              std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  constexpr inline expr_type operator()(scalar_one<value_type> const &) {
    return visit(scalar_one_sub<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                  std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  template <typename Type>
  constexpr inline expr_type operator()([[maybe_unused]] Type const &rhs) {
    return visit(sub_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                               std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  // 0 - expr
  constexpr inline expr_type operator()(scalar_zero<value_type> const &) {
    return make_expression<scalar_negative<value_type>>(
        std::forward<ExprRHS>(m_rhs));
  }

  // - expr_lhs - expr_rhs --> -(expr_lhs+expr_rhs)
  constexpr inline expr_type
  operator()(scalar_negative<value_type> const &lhs) {
    return make_expression<scalar_negative<value_type>>(
        lhs.expr() + std::forward<ExprRHS>(m_rhs));
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

} // namespace simplifier
} // namespace numsim::cas

// namespace numsim::cas {

// template <typename ExprLHS, typename ExprRHS> class scalar_simplifier_sub {
// public:
//   using value_type = typename std::remove_reference_t<
//       std::remove_const_t<ExprLHS>>::value_type;
//   using expr_type = expression_holder<scalar_expression<value_type>>;

//  scalar_simplifier_sub(ExprLHS &&lhs, ExprRHS &&rhs)
//      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs))
//      {}

//  auto operator()(scalar<value_type> &lhs, scalar<value_type> & rhs) {
//    if(&lhs == &rhs){
//      return get_scalar_zero<value_type>();
//    }
//    return default_sub();
//  }

//  auto operator()(scalar_constant<value_type> &lhs,
//                  scalar_constant<value_type> &rhs) {
//    return make_expression<scalar_constant<value_type>>(lhs() - rhs());
//  }

//  auto operator()(scalar_one<value_type> &,
//                  scalar_constant<value_type> &rhs) {
//    return make_expression<scalar_constant<value_type>>(1 - rhs());
//  }

//  auto operator()(scalar_constant<value_type> &lhs,
//                  scalar_one<value_type> &) {
//    return make_expression<scalar_constant<value_type>>(lhs() - 1);
//  }

//  auto operator()(scalar_one<value_type> &,
//                  scalar_one<value_type> &) {
//    return make_expression<scalar_constant<value_type>>(2);
//  }

////  template<typename Scalar>
////    requires (!isScalarConstant<Scalar> || !isScalarZero<Scalar>)
////  auto operator()([[maybe_unused]] scalar_one<value_type> &lhs,
///[[maybe_unused]] Scalar &rhs) { /    return default_sub(); /  }

////  template<typename Scalar>
////    requires (!isScalarConstant<Scalar> || !isScalarZero<Scalar>)
////  auto operator()([[maybe_unused]] Scalar &lhs, [[maybe_unused]]
/// scalar_one<value_type> &rhs) { /    return default_sub(); /  }

//  auto operator()([[maybe_unused]] scalar_sub<value_type> &lhs,
//                  [[maybe_unused]] scalar_constant<value_type> &rhs) {
//    auto sub{make_expression<scalar_sub<value_type>>(lhs)};
//    auto coeff{m_rhs + lhs.coeff()};
//    sub.template get<scalar_sub<value_type>>().set_coeff(std::move(coeff));
//    return std::move(sub);
//  }

//  auto operator()([[maybe_unused]] scalar_constant<value_type> &lhs,
//                  [[maybe_unused]] scalar_sub<value_type> &rhs) {
//    auto sub{make_expression<scalar_sub<value_type>>(rhs)};
//    auto coeff{m_rhs + rhs.coeff()};
//    sub.template get<scalar_sub<value_type>>().set_coeff(std::move(coeff));
//    return std::move(sub);
//  }

//  auto operator()([[maybe_unused]] scalar<value_type> &lhs,
//                  [[maybe_unused]] scalar_constant<value_type> &rhs) {
//    auto sub{make_expression<scalar_sub<value_type>>()};
//    sub.template get<scalar_sub<value_type>>().set_coeff(m_rhs);
//    sub.template get<scalar_sub<value_type>>().push_back(m_lhs);
//    return std::move(sub);
//  }

//  auto operator()([[maybe_unused]] scalar_constant<value_type> &lhs,
//                  [[maybe_unused]] scalar<value_type> &rhs) {
//    auto sub{make_expression<scalar_sub<value_type>>()};
//    sub.template get<scalar_sub<value_type>>().set_coeff(m_lhs);
//    sub.template get<scalar_sub<value_type>>().push_back(m_rhs);
//    return std::move(sub);
//  }

//  auto operator()([[maybe_unused]] scalar_sub<value_type> &lhs,
//                  [[maybe_unused]] scalar<value_type> &rhs) {
//    auto sub{make_expression<scalar_sub<value_type>>(lhs)};
//    sub.template get<scalar_sub<value_type>>().push_back(m_rhs);
//    return std::move(sub);
//  }

//  auto operator()([[maybe_unused]] scalar<value_type> &lhs, [[maybe_unused]]
//  scalar_sub<value_type> &rhs) {
//    auto sub{make_expression<scalar_sub<value_type>>(rhs)};
//    sub.template get<scalar_sub<value_type>>().push_back(m_lhs);
//    return std::move(sub);
//  }

//  /// -expr - expr --> -2*expr
//  auto operator()(scalar_sub<value_type> &lhs, scalar_sub<value_type> &rhs) {
//    const auto &hash_rhs{rhs.hash_value()};
//    const auto &hash_lhs{lhs.hash_value()};
//    if (hash_rhs == hash_lhs) {
//      const auto fac_lhs{get_coefficient(lhs, 1.0)};
//      const auto fac_rhs{get_coefficient(rhs, 1.0)};
//      auto expr{make_expression<scalar_sub<value_type>>(lhs)};
//      auto &sub{expr.template get<scalar_sub<value_type>>()};
//      sub.set_coeff(
//          make_expression<scalar_constant<value_type>>(fac_lhs - fac_rhs));
//      return std::move(expr);
//    }/*else{
//      auto expr{make_expression<scalar_sub<value_type>>()};
//      auto &sub{expr.template get<scalar_sub<value_type>>()};
//      sub.reserve(lhs.size() + rhs.size());
//      for(auto &expr_out : lhs.hash_map() | std::views::values){
//        sub.push_back(expr);
//      }
//      for(auto &expr_out : rhs.hash_map() | std::views::values){
//        sub.push_back(expr);
//      }
//      return std::move(expr);
//    }*/
//    return default_sub();
//  }

//  auto operator()(scalar_negative<value_type> &, scalar_one<value_type> &) {
//    return default_sub();
//  }

//  template<typename Scalar>
//  auto operator()(scalar_negative<value_type> &lhs, scalar_sub<value_type>
//  &rhs) {
//    auto expr{make_expression<scalar_sub<value_type>>(rhs)};
//    auto &sub{expr.template get<scalar_sub<value_type>>()};
//    sub.push_back(m_lhs.expr());
//    return std::move(expr);
//  }

//  /// (-x) - x --> -2*x
//  /// (-x) - y --> - x - y
//  template<typename Scalar>
//  requires(!isScalarZero<Scalar>)
//  auto operator()(scalar_negative<value_type> &lhs, Scalar &rhs) {
//    const auto &hash_rhs{rhs.hash_value()};
//    const auto &hash_lhs{lhs.expr().get().hash_value()};
//    if (hash_rhs == hash_lhs) {
//      auto expr{make_expression<scalar_mul<value_type>>()};
//      auto &mul{expr.template get<scalar_mul<value_type>>()};
//      mul.set_coeff(
//          make_expression<scalar_constant<value_type>>(2));
//      mul.push_back(lhs.expr());
//      return make_expression<scalar_negative<value_type>>(std::move(expr));
//    }else{
//      auto expr{make_expression<scalar_sub<value_type>>()};
//      auto &sub{expr.template get<scalar_sub<value_type>>()};
//      sub.push_back(m_lhs);
//      sub.push_back(m_rhs);
//      return std::move(expr);
//    }
//  }

//////         //    /// -expr - expr --> -2*expr
//////         //    auto operator()(scalar_sub<value_type> &lhs,
/// scalar_sub<value_type> &rhs) {
//////         //      const auto &hash_rhs{rhs.hash_value()};
//////         //      const auto &hash_lhs{lhs.hash_value()};
//////         //      if (hash_rhs == hash_lhs) {
//////         //      const auto fac_lhs{get_coefficient(lhs, 1.0)};
//////         //      const auto fac_rhs{get_coefficient(rhs, 1.0)};
//////         //      auto expr{make_expression<scalar_mul<value_type>>(lhs)};
//////         //      auto &mul{expr.template get<scalar_mul<value_type>>()};
//////         //      mul.set_coeff(
//////         //          make_expression<scalar_constant<value_type>>(fac_lhs
///+ fac_rhs));
//////         //      return std::move(expr);
//////         //      }
//////         //      return default_add();
//////         //    }

//////         //  auto operator()(scalar_add<value_type> &lhs,
/// scalar<value_type> &rhs) {
//////         //    /// check if expr_rhs == expr_lhs -->
///(factor_lhs+factor_rhs)*expr_lhs
//////         //    if (lhs.hash_value() == rhs.hash_value()) {
//////         //      auto expr{make_expression<scalar_mul<value_type>>()};
//////         //      auto &mul{expr.template get<scalar_mul<value_type>>()};
//////         //      mul.push_back(m_rhs);
//////         //
/// mul.set_coeff(make_expression<scalar_constant<value_type>>(2));
//////         //      const auto coeff_lhs{get_coefficient(lhs, 0.0)};
//////         //      if (coeff_lhs != 0) {
//////         //        auto
/// expr_add{make_expression<scalar_add<value_type>>()};
//////         //        expr_add.template
/// get<scalar_add<value_type>>().set_coeff(
//////         // make_expression<scalar_constant<value_type>>(coeff_lhs));
//////         //        expr_add.template
/// get<scalar_add<value_type>>().push_back(
//////         //            std::move(expr));
//////         //        return std::move(expr_add);
//////         //      }
//////         //      return std::move(expr);
//////         //    }

//////         //           /// check if sub_exp == expr_rhs for sub_exp \in
/// expr_lhs
//////         //    auto pos{lhs.hash_map().find(rhs.hash_value())};
//////         //    if (pos != lhs.hash_map().end()) {
//////         //      auto expr{std::visit(
//////         //          scalar_simplifier_add<expr_type &, expr_type
///&>(pos->second, m_rhs),
//////         //          *pos->second, *m_rhs)};
//////         //      if (expr) {
//////         //        auto
/// expr_add{make_expression<scalar_add<value_type>>(lhs)};
//////         //        auto &add{expr_add.template
/// get<scalar_add<value_type>>()};
//////         //        add.hash_map().erase(rhs.hash_value());
//////         //        add.push_back(std::move(expr));
//////         //        return std::move(expr_add);
//////         //      }
//////         //    }

//////         //           /// no equal expr or sub_expr
//////         //    lhs.push_back(m_rhs);
//////         //    return m_lhs;
//////         //  }

//////         //  template <typename Scalar>
//////         //  auto operator()(Scalar &lhs, scalar_constant<value_type>
///&rhs) {
//////         //    auto mul{make_expression<scalar_mul<value_type>>()};
//////         //    mul.template
/// get<scalar_mul<value_type>>().set_coeff(std::forward<ExprRHS>(m_rhs));
//////         //    mul.template
/// get<scalar_mul<value_type>>().push_back(std::forward<ExprLHS>(m_lhs));
//////         //    return mul;
//////         //  }

//////         //  template <typename Scalar>
//////         //  auto operator()(scalar_constant<value_type> &lhs, Scalar
///&rhs) {
//////         //    auto mul{make_expression<scalar_mul<value_type>>()};
//////         //    mul.template
/// get<scalar_mul<value_type>>().push_back(m_rhs);
//////         //    mul.template
/// get<scalar_mul<value_type>>().set_coeff(m_lhs);
//////         //    return mul;
//////         //    //return std::forward<ExprRHS>(m_rhs) +
/// std::forward<ExprLHS>(m_lhs);
//////         //  }

//////         //  auto operator()(scalar_mul<value_type> &lhs,
/// scalar_one<value_type> & rhs) {
//////         //    return std::forward<ExprLHS>(m_lhs);
//////         //  }

//////         //  auto operator()(scalar_one<value_type> &lhs,
/// scalar_mul<value_type> & rhs) {
//////         //    return std::forward<ExprRHS>(m_rhs);
//////         //  }

//////         //  auto operator()(scalar_constant<value_type> &lhs,
/// scalar_one<value_type> & rhs) {
//////         //    return std::forward<ExprLHS>(m_lhs);
//////         //  }

//////         //  auto operator()(scalar_one<value_type> &lhs,
/// scalar_constant<value_type> & rhs) {
//////         //    return std::forward<ExprRHS>(m_rhs);
//////         //  }

//////         //  auto operator()(scalar_mul<value_type> &lhs,
/// scalar_zero<value_type> & rhs) {
//////         //    return std::forward<ExprRHS>(m_rhs);
//////         //  }

//////         //  auto operator()(scalar_zero<value_type> &lhs,
/// scalar_mul<value_type> & rhs) {
//////         //    return std::forward<ExprLHS>(m_lhs);
//////         //  }

//////         //  auto operator()(scalar<value_type> &lhs,
/// scalar_zero<value_type> & rhs) {
//////         //    return std::forward<ExprRHS>(m_rhs);
//////         //  }

//////         //  auto operator()(scalar_zero<value_type> &lhs,
/// scalar<value_type> & rhs) {
//////         //    return std::forward<ExprLHS>(m_lhs);
//////         //  }

//////         //  auto operator()(scalar_constant<value_type> &lhs,
/// scalar_zero<value_type> & rhs) {
//////         //    return std::forward<ExprRHS>(m_rhs);
//////         //  }

//////         //  auto operator()(scalar_zero<value_type> &lhs,
/// scalar_constant<value_type> & rhs) {
//////         //    return std::forward<ExprLHS>(m_lhs);
//////         //  }

//////         //  template <typename Scalar>
//////         //  auto operator()(scalar_mul<value_type> &lhs, Scalar & rhs) {
//////         //    auto expr{make_expression<scalar_mul<value_type>>(lhs)};
//////         //    expr.template
/// get<scalar_mul<value_type>>().push_back(std::forward<ExprRHS>(m_rhs));
//////         //    return expr;
//////         //  }

//////         //  //  template <typename Scalar>
//////         //  //  expr_type operator()(Scalar &lhs, scalar_mul<value_type>
///&rhs) {
//////         //  //    return std::forward<ExprRHS>(m_rhs) +
/// std::forward<ExprLHS>(m_lhs);
//////         //  //  }

//  //0 - expr --> -expr
//  template<typename Scalar>
//  auto operator()([[maybe_unused]] scalar_zero<value_type> &lhs,
//  [[maybe_unused]] Scalar &rhs) {
//    return make_expression<scalar_negative<value_type>>(m_rhs);
//  }

//  //expr - 0 --> expr
//  template<typename Scalar>
//  auto operator()([[maybe_unused]] Scalar &lhs, [[maybe_unused]]
//  scalar_zero<value_type> &rhs) {
//    return m_lhs;
//  }

//  //0 - 0 --> 0
//  auto operator()([[maybe_unused]] scalar_zero<value_type> &lhs,
//  [[maybe_unused]] scalar_zero<value_type> &rhs) {
//    return m_lhs;
//  }

////  template<typename Scalar> requires(!isScalarConstant<Scalar> &&
///!isScalarZero<Scalar>) /  auto operator()([[maybe_unused]]
/// scalar_one<value_type> &lhs, [[maybe_unused]] Scalar &rhs) { /    return
/// default_sub(); /  }

////  template<typename Scalar> requires(!isScalarConstant<Scalar> &&
///!isScalarZero<Scalar>) /  auto operator()([[maybe_unused]] Scalar &lhs,
///[[maybe_unused]] scalar_one<value_type> &rhs) { /    return default_sub(); /
///}

////  //expr - 0 --> expr
////  auto operator()([[maybe_unused]] scalar_sub<value_type> &lhs,
///[[maybe_unused]] scalar_zero<value_type> &rhs) { /    return m_lhs; /  }

////  //0 - (-expr) --> expr
////  auto operator()([[maybe_unused]] scalar_zero<value_type> &lhs,
///[[maybe_unused]] scalar_negative<value_type> &rhs) { /    return rhs.expr();
////  }

////  //-expr - 0 --> -expr
////  auto operator()([[maybe_unused]] scalar_negative<value_type> &lhs,
///[[maybe_unused]] scalar_zero<value_type> &rhs) { /    return m_lhs; /  }

////  //symbol - 0 --> symbol
////  auto operator()([[maybe_unused]] scalar<value_type> &lhs, [[maybe_unused]]
/// scalar_zero<value_type> &rhs) { /    return m_lhs; /  }

////  //0 - symbol --> -symbol
////  auto operator()([[maybe_unused]] scalar_zero<value_type> &lhs,
///[[maybe_unused]] scalar<value_type> &rhs) { /    return
/// make_expression<scalar_negative<value_type>>(m_rhs); /  }

////  auto operator()([[maybe_unused]] scalar_constant<value_type> & lhs,
///[[maybe_unused]] scalar_div<value_type> & rhs) { /    return default_sub();
////  }

////  auto operator()([[maybe_unused]] scalar_constant<value_type> & lhs,
///[[maybe_unused]] scalar_add<value_type> & rhs) { /    return default_sub();
////  }

////  auto operator()([[maybe_unused]] scalar_constant<value_type> & lhs,
///[[maybe_unused]] scalar_mul<value_type> & rhs) { /    return default_sub();
////  }

////  auto operator()([[maybe_unused]] scalar_constant<value_type> & lhs,
///[[maybe_unused]] scalar_negative<value_type> & rhs) { /    return
/// default_sub(); /  }

//  //expr - expr --> expr - expr
//  template <typename ScalarLHS, typename ScalarRHS>
//  requires (! (isScalarConstant<ScalarLHS> && isScalarOne<ScalarRHS>) ||
//             ! (isScalarConstant<ScalarRHS> && isScalarOne<ScalarLHS>))
//  auto operator()([[maybe_unused]] ScalarLHS & lhs, [[maybe_unused]] ScalarRHS
//  & rhs) {
//    std::cout<<"default op"<<std::endl;
//    std::cout<<std::boolalpha<<isScalarConstant<std::remove_reference_t<ScalarLHS>><<"
//    "<<isScalarOne<std::remove_reference_t<ScalarRHS>><<std::endl;
//    std::cout<<std::boolalpha<<isScalarConstant<std::remove_reference_t<ScalarRHS>><<"
//    "<<isScalarOne<std::remove_reference_t<ScalarLHS>><<std::endl;
//    std::cout<<std::is_same_v<std::remove_cvref_t<ScalarLHS>,
//    scalar_constant<value_type>><<std::endl;
//    std::cout<<std::is_same_v<std::remove_cvref_t<ScalarRHS>,
//    scalar_constant<value_type>><<std::endl;
//    std::cout<<std::is_same_v<std::remove_cvref_t<ScalarLHS>,
//    scalar_one<value_type>><<std::endl;
//    std::cout<<std::is_same_v<std::remove_cvref_t<ScalarRHS>,
//    scalar_one<value_type>><<std::endl; return default_sub();
//  }

// private:
//   template <typename _Expr, typename _ValueType>
//   constexpr auto get_coefficient(_Expr const &expr, _ValueType const &value)
//   {
//     if constexpr (is_detected_v<has_coefficient, _Expr>) {
//       auto func{[&](auto const &coeff) {
//         return coeff.is_valid() ? coeff.template
//         get<scalar_constant<value_type>>()()
//                      : value;
//       }};
//       return func(expr.coeff());
//     }
//     return value;
//   }

//  auto default_sub() {
//    auto sub{make_expression<scalar_sub<value_type>>()};
//    sub.template get<scalar_sub<value_type>>().push_back(
//        std::forward<ExprLHS>(m_lhs));
//    sub.template get<scalar_sub<value_type>>().push_back(
//        std::forward<ExprRHS>(m_rhs));
//    return sub;
//  }

//  ExprLHS m_lhs;
//  ExprRHS m_rhs;
//};

//}

#endif // SCALAR_SIMPLIFIER_SUB_H
