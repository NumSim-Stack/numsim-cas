#ifndef TENSOR_SIMPLIFIER_MUL_H
#define TENSOR_SIMPLIFIER_MUL_H

#include "../../expression_holder.h"
#include "../../numsim_cas_forward.h"
#include "../../numsim_cas_type_traits.h"
#include "../../operators.h"
#include "../../scalar/scalar_constant.h"
#include "../functions/tensor_pow.h"
#include "../operators/tensor/tensor_mul.h"
#include "../tensor_std.h"
#include <set>
#include <type_traits>

namespace numsim::cas {
namespace tensor_detail {
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

  //    //if(rhs_constant){
  //    //  add.set_coeff(m_rhs);
  //    //}else{
  //      add.push_back(m_rhs);
  //    //}
  //    return std::move(add_new);
  //  }

  // expr * 0 --> 0
  constexpr inline expr_type operator()(tensor_zero<value_type> const &) {
    return std::forward<ExprRHS>(m_rhs);
  }

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
    return get_default();
  }

protected:
  auto get_default() {
    if (m_lhs.get().hash_value() == m_rhs.get().hash_value()) {
      return std::pow(std::forward<ExprLHS>(m_lhs), 2);
    }
    // const auto lhs_constant{is_same<tensor_constant<value_type>>(m_lhs)};
    // const auto rhs_constant{is_same<tensor_constant<value_type>>(m_rhs)};
    auto mul_new{make_expression<tensor_mul<value_type>>(m_lhs.get().dim(),
                                                         m_rhs.get().rank())};
    auto &add{mul_new.template get<tensor_mul<value_type>>()};
    // if(lhs_constant){
    //   add.set_coeff(m_lhs);
    // }else{
    add.push_back(m_lhs);
    add.push_back(m_rhs);
    //}
    return mul_new;
  }

protected:
  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

template <typename ExprLHS, typename ExprRHS>
class tensor_pow_mul final : public mul_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;
  using base = mul_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_default;

  tensor_pow_mul(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor_pow<value_type>>()} {}

  auto operator()([[maybe_unused]] tensor<value_type> const &rhs) {
    if (lhs.expr_lhs().get().hash_value() == rhs.hash_value()) {
      const auto rhs_expr{lhs.expr_rhs() + get_scalar_one<value_type>()};
      return make_expression<tensor_pow<value_type>>(lhs.expr_lhs(),
                                                     std::move(rhs_expr));
    }
    return get_default();
  }

  auto operator()([[maybe_unused]] tensor_pow<value_type> const &rhs) {
    if (lhs.hash_value() == rhs.hash_value()) {
      const auto rhs_expr{lhs.expr_rhs() + rhs.expr_rhs()};
      return make_expression<tensor_pow<value_type>>(lhs.expr_lhs(),
                                                     std::move(rhs_expr));
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_pow<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class kronecker_delta_mul final : public mul_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;
  using base = mul_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_default;

  kronecker_delta_mul(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<kronecker_delta<value_type>>()} {}

  // I_ij*expr_jkmnop.... --> expr_ikmnop....
  template <typename Expr> auto operator()([[maybe_unused]] Expr const &rhs) {
    return std::forward<ExprRHS>(m_rhs);
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  kronecker_delta<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class symbol_mul final : public mul_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;
  using base = mul_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_default;

  symbol_mul(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor<value_type>>()} {}

  /// X*X --> pow(X,2)
  constexpr inline expr_type operator()(tensor<value_type> const &rhs) {
    if (&lhs == &rhs) {
      return std::pow(std::forward<ExprLHS>(m_lhs), 2);
    }
    return get_default();
  }

  /// X * pow(X,expr) --> pow(X,expr+1)
  constexpr inline expr_type operator()(tensor_pow<value_type> const &rhs) {
    if (lhs.hash_value() == rhs.expr_lhs().get().hash_value()) {
      return std::pow(m_lhs, rhs.expr_rhs() + get_scalar_one<value_type>());
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class n_ary_mul final : public mul_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;
  using base = mul_default<ExprLHS, ExprRHS>;
  using base::operator();

  n_ary_mul(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<tensor_mul<value_type>>()} {}

  // check if last element == rhs; if not just pushback element
  template <typename Expr> auto operator()([[maybe_unused]] Expr const &rhs) {
    auto expr_mul{make_expression<tensor_mul<value_type>>(lhs)};
    auto &mul{expr_mul.template get<tensor_mul<value_type>>()};
    if (lhs.data().back() == m_rhs) {
      auto last_element{lhs.data().back()};
      mul.data().erase(mul.data().end() - 1);
      return std::move(expr_mul) * (last_element * m_rhs);
    }
    mul.push_back(m_rhs);
    return expr_mul;
  }

  // merge to tensor_mul objects
  auto operator()([[maybe_unused]] tensor_mul<value_type> const &rhs) {
    auto expr_mul{make_expression<tensor_mul<value_type>>(lhs)};
    auto &mul{expr_mul.template get<tensor_mul<value_type>>()};
    for (const auto &expr : rhs.data()) {
      mul.push_back(expr);
    }
    return expr_mul;
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_mul<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS> struct mul_base {
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<tensor_expression<value_type>>;

  mul_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  constexpr inline expr_type operator()(tensor<value_type> const &) {
    const auto &expr{*m_rhs};
    return visit(symbol_mul<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                              std::forward<ExprRHS>(m_rhs)),
                 expr);
  }

  constexpr inline expr_type operator()(tensor_zero<value_type> const &) {
    return std::forward<ExprLHS>(m_lhs);
  }

  constexpr inline expr_type operator()(tensor_pow<value_type> const &) {
    const auto &expr{*m_rhs};
    return visit(tensor_pow_mul<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                  std::forward<ExprRHS>(m_rhs)),
                 expr);
  }

  constexpr inline expr_type operator()(kronecker_delta<value_type> const &) {
    return visit(
        kronecker_delta_mul<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                              std::forward<ExprRHS>(m_rhs)),
        *m_rhs);
  }

  constexpr inline expr_type operator()(tensor_mul<value_type> const &) {
    return visit(n_ary_mul<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                             std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  template <typename Expr> constexpr inline expr_type operator()(Expr const &) {
    const auto &expr{*m_lhs};
    return visit(mul_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                               std::forward<ExprRHS>(m_rhs)),
                 expr);
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

} // namespace simplifier
} // namespace tensor_detail
} // namespace numsim::cas

#endif // TENSOR_SIMPLIFIER_MUL_H
