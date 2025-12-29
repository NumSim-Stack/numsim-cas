#ifndef SCALAR_SIMPLIFIER_ADD_H
#define SCALAR_SIMPLIFIER_ADD_H

#include "../../basic_functions.h"
#include "../../expression_holder.h"
#include "../../functions.h"
#include "../../numsim_cas_type_traits.h"
#include "../../operators.h"
#include "../scalar_functions.h"
#include "../scalar_globals.h"
#include <functional>
#include <optional>
#include <set>
#include <type_traits>

namespace numsim::cas {
namespace simplifier {

template <typename ExprLHS, typename ExprRHS> class add_default {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;

  add_default(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  auto get_default() {
    const auto lhs_constant{is_same<scalar_constant<value_type>>(m_lhs) ||
                            is_same<scalar_one<value_type>>(m_lhs)};
    const auto rhs_constant{is_same<scalar_constant<value_type>>(m_rhs) ||
                            is_same<scalar_one<value_type>>(m_rhs)};
    auto add_new{make_expression<scalar_add<value_type>>()};
    if (lhs_constant) {
      auto &add{add_new.template get<scalar_add<value_type>>()};
      add.set_coeff(m_lhs);
    } else {
      add_new.template get<scalar_add<value_type>>().push_back(m_lhs);
    }

    if (rhs_constant) {
      auto &add{add_new.template get<scalar_add<value_type>>()};
      add.set_coeff(m_rhs);
    } else {
      add_new.template get<scalar_add<value_type>>().push_back(m_rhs);
    }
    return std::move(add_new);
  }

  template <typename Expr> constexpr inline expr_type operator()(Expr const &) {
    return get_default();
  }

  constexpr inline expr_type operator()(scalar_zero<value_type> const &) {
    return m_lhs;
  }

  constexpr inline expr_type
  operator()(scalar_negative<value_type> const &expr) {
    if (m_lhs.get().hash_value() == expr.expr().get().hash_value()) {
      return get_scalar_zero<value_type>();
    }
    return get_default();
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
class constant_add final : public add_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_coefficient;

  constant_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_constant<value_type>>()} {}

  constexpr inline expr_type
  operator()(scalar_constant<value_type> const &rhs) {
    const auto value{lhs() + rhs()};
    return make_expression<scalar_constant<value_type>>(value);
  }

  constexpr inline expr_type
  operator()([[maybe_unused]] scalar_add<value_type> const &rhs) {
    auto add_expr{make_expression<scalar_add<value_type>>(rhs)};
    auto &add{add_expr.template get<scalar_add<value_type>>()};
    auto coeff{get_coefficient(add, value_type{0}) + base::m_lhs};
    add.set_coeff(std::move(coeff));
    return std::move(add_expr);
  }

  constexpr inline expr_type operator()(scalar_one<value_type> const &) {
    const auto value{lhs() + value_type{1}};
    return make_expression<scalar_constant<value_type>>(value);
  }

private:
  scalar_constant<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class n_ary_add final : public add_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_coefficient;
  using base::get_default;

  n_ary_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_add<value_type>>()} {}

  constexpr inline expr_type
  operator()([[maybe_unused]] scalar_constant<value_type> const &rhs) {
    auto add_expr{make_expression<scalar_add<value_type>>(lhs)};
    auto &add{add_expr.template get<scalar_add<value_type>>()};
    auto coeff{get_coefficient(add, value_type{0}) + m_rhs};
    add.set_coeff(std::move(coeff));
    return std::move(add_expr);
  }

  constexpr inline expr_type
  operator()([[maybe_unused]] scalar_one<value_type> const &) {
    auto add_expr{make_expression<scalar_add<value_type>>(lhs)};
    auto &add{add_expr.template get<scalar_add<value_type>>()};
    auto coeff{make_expression<scalar_constant<value_type>>(
        get_coefficient(add, value_type{0}) + value_type{1})};
    // auto coeff{add.coeff() + m_rhs};
    add.set_coeff(std::move(coeff));
    return add_expr;
  }

  auto operator()([[maybe_unused]] scalar<value_type> const &rhs) {
    /// do a deep copy of data
    auto expr_add{make_expression<scalar_add<value_type>>(lhs)};
    auto &add{expr_add.template get<scalar_add<value_type>>()};
    /// check if sub_exp == expr_rhs for sub_exp \in expr_lhs
    auto pos{lhs.hash_map().find(m_rhs)};
    if (pos != lhs.hash_map().end()) {
      auto expr{binary_scalar_add_simplify(pos->second, m_rhs)};
      add.hash_map().erase(m_rhs);
      add.push_back(expr);
      return expr_add;
    }
    /// no equal expr or sub_expr
    add.push_back(m_rhs);
    return expr_add;
  }

  // merge two expression
  auto operator()(scalar_add<value_type> const &rhs) {
    auto expr{make_expression<scalar_add<value_type>>()};
    auto &add{expr.template get<scalar_add<value_type>>()};
    merge_add(lhs, rhs, add);
    return expr;
  }

  auto operator()(scalar_negative<value_type> const &rhs) {
    const auto pos{lhs.hash_map().find(rhs.expr())};
    if (pos != lhs.hash_map().end()) {
      auto expr{make_expression<scalar_add<value_type>>(lhs)};
      auto &add{expr.template get<scalar_add<value_type>>()};
      add.hash_map().erase(rhs.expr());
      return expr;
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_add<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class n_ary_mul_add final : public add_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_coefficient;
  using base::get_default;

  n_ary_mul_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_mul<value_type>>()} {}

  // constant*expr + expr --> (constant+1)*expr
  auto operator()([[maybe_unused]] scalar<value_type> const &rhs) {
    // const auto &hash_lhs{lhs.hash_value()};
    const auto pos{lhs.hash_map().find(m_rhs)};
    if (pos != lhs.hash_map().end() && lhs.hash_map().size() == 1) {
      auto expr{make_expression<scalar_mul<value_type>>(lhs)};
      auto &mul{expr.template get<scalar_mul<value_type>>()};
      mul.set_coeff(make_expression<scalar_constant<value_type>>(
          get_coefficient(lhs, value_type{1}) + value_type{1}));
      return std::move(expr);
    }
    return get_default();
  }

  /// expr + expr --> 2*expr
  auto operator()(scalar_mul<value_type> const &rhs) {
    const auto &hash_rhs{rhs.hash_value()};
    const auto &hash_lhs{lhs.hash_value()};
    if (hash_rhs == hash_lhs) {
      const auto fac_lhs{get_coefficient(lhs, value_type{1})};
      const auto fac_rhs{get_coefficient(rhs, value_type{1})};
      auto expr{make_expression<scalar_mul<value_type>>(lhs)};
      auto &mul{expr.template get<scalar_mul<value_type>>()};
      mul.set_coeff(
          make_expression<scalar_constant<value_type>>(fac_lhs + fac_rhs));
      return std::move(expr);
    }
    return get_default();
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_mul<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS>
class symbol_add final : public add_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_coefficient;
  using base::get_default;

  symbol_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar<value_type>>()} {}

  /// x+x --> 2*x
  constexpr inline expr_type operator()(scalar<value_type> const &rhs) {
    if (&lhs == &rhs) {
      auto mul{make_expression<scalar_mul<value_type>>()};
      mul.template get<scalar_mul<value_type>>().set_coeff(
          make_expression<scalar_constant<value_type>>(2));
      mul.template get<scalar_mul<value_type>>().push_back(m_rhs);
      return std::move(mul);
    }
    return get_default();
  }

  constexpr inline expr_type operator()(scalar_mul<value_type> const &rhs) {
    // const auto &hash_rhs{rhs.hash_value()};
    const auto pos{rhs.hash_map().find(m_lhs)};
    if (pos != rhs.hash_map().end() && rhs.hash_map().size() == 1) {
      auto expr{make_expression<scalar_mul<value_type>>(rhs)};
      auto &mul{expr.template get<scalar_mul<value_type>>()};
      mul.set_coeff(make_expression<scalar_constant<value_type>>(
          get_coefficient(rhs, value_type{1}) + value_type{1}));
      return std::move(expr);
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
class add_negative final : public add_default<ExprLHS, ExprRHS> {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;
  using base = add_default<ExprLHS, ExprRHS>;
  using base::operator();
  using base::get_coefficient;
  using base::get_default;

  add_negative(ExprLHS &&lhs, ExprRHS &&rhs)
      : base(std::forward<ExprLHS>(lhs), std::forward<ExprRHS>(rhs)),
        lhs{base::m_lhs.template get<scalar_negative<value_type>>()} {}

  // (-lhs) + (-rhs) --> -(lhs+rhs)
  constexpr inline expr_type
  operator()(scalar_negative<value_type> const &rhs) {
    return -(lhs.expr() + rhs.expr());
  }

private:
  using base::m_lhs;
  using base::m_rhs;
  scalar_negative<value_type> const &lhs;
};

template <typename ExprLHS, typename ExprRHS> struct add_base {
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;

  add_base(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  constexpr inline expr_type operator()(scalar_constant<value_type> const &) {
    return visit(constant_add<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  constexpr inline expr_type operator()(scalar_add<value_type> const &) {
    return visit(n_ary_add<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                             std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  constexpr inline expr_type operator()(scalar_mul<value_type> const &) {
    return visit(n_ary_mul_add<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                 std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  constexpr inline expr_type operator()(scalar<value_type> const &) {
    return visit(symbol_add<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                              std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  constexpr inline expr_type operator()(scalar_negative<value_type> const &) {
    return visit(add_negative<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                                std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  constexpr inline expr_type operator()(scalar_zero<value_type> const &) {
    return std::forward<ExprRHS>(m_rhs);
  }

  template <typename Type> constexpr inline expr_type operator()(Type const &) {
    return visit(add_default<ExprLHS, ExprRHS>(std::forward<ExprLHS>(m_lhs),
                                               std::forward<ExprRHS>(m_rhs)),
                 *m_rhs);
  }

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};
} // namespace simplifier

// template<typename T>
// class SimplifyRuleRegistry {
// public:
//   using value_type = T;
//   using expr_t = expression_holder<scalar_expression<value_type>>;
//   using SimplifyRuleFn = std::function<expr_t(expr_t const&, expr_t const&)>;
//   using RuleMap = std::unordered_map<std::pair<type_id, type_id>,
//   SimplifyRuleFn>;

//  static SimplifyRuleRegistry& instance() {
//    static SimplifyRuleRegistry inst;
//    return inst;
//  }

//  template<typename LHS, typename RHS>
//  static void register_rule(SimplifyRuleFn fn) {
//    instance().rule_map_[{LHS::get_id(), RHS::get_id()}] = std::move(fn);
//  }

//  static SimplifyRuleFn lookup(type_id lhs, type_id rhs) {
//    auto it = instance().rule_map_.find({lhs, rhs});
//    return it != instance().rule_map_.end() ? it->second : nullptr;
//  }

// private:
//   RuleMap rule_map_;
// };

// template<typename T>
// using scalar_simplify_rules = simplify_rule_registry<T, scalar_expression>;

// template<typename ValueType, typename TypeLHS, typename TypeRHS, typename
// Func> static auto register_binary_simplify_scalar(Func function){
//   scalar_simplify_rules<ValueType>::instance().template
//   register_rule<TypeLHS, TypeRHS>(std::forward<Func>(function)); return true;
// }

// template<typename ValueType, template<typename > class... Args, typename
// Func> static auto register_binary_simplify_scalar_var(Func function){
//   scalar_simplify_rules<ValueType>::instance().template
//   register_rule<Args<ValueType>...>(std::forward<Func>(function)); return
//   true;
// }

////namespace scalar_rules {
///// constant + constant = constant
// template<typename ValueType>
// auto add_constant_constant(expression_holder<scalar_expression<ValueType>>
// const&lhs,
//                           expression_holder<scalar_expression<ValueType>>
//                           const&rhs){
//   using type = scalar_constant<ValueType>;
//   const auto value{lhs.template get<type>()() + rhs.template get<type>()()};
//   return make_expression<scalar_constant<ValueType>>(value);
// }

///// constant - constant = constant
// template<typename ValueType>
// auto sub_constant_constant(expression_holder<scalar_expression<ValueType>>
// const&lhs,
//                            expression_holder<scalar_expression<ValueType>>
//                            const&rhs){
//   using type = scalar_constant<ValueType>;
//   const auto value{lhs.template get<type>()() - rhs.template get<type>()()};
//   return make_expression<scalar_constant<ValueType>>(value);
// }

///// constant + negative
// template<typename ValueType>
// auto add_constant_negative(expression_holder<scalar_expression<ValueType>>
// const&lhs,
//                            expression_holder<scalar_expression<ValueType>>
//                            const&rhs){
//   using constant = scalar_constant<ValueType>;
//   using negative = scalar_negative<ValueType>;
//   using add = scalar_add<ValueType>;
//   const auto
//   key{scalar_simplify_rules<ValueType>::instance().get_key(constant::get_id(),
//   add::get_id(), negative::get_id(), get_id(rhs))}; return
//   scalar_simplify_rules<ValueType>::instance().run(key, lhs, rhs);
// }

///// negative + constant
// template<typename ValueType>
// auto add_negative_constant(expression_holder<scalar_expression<ValueType>>
// const&lhs,
//                            expression_holder<scalar_expression<ValueType>>
//                            const&rhs){
//   using constant = scalar_constant<ValueType>;
//   using negative = scalar_negative<ValueType>;
//   using add = scalar_add<ValueType>;
//   const auto
//   key{scalar_simplify_rules<ValueType>::instance().get_key(negative::get_id(),
//   get_id(lhs), add::get_id(), constant::get_id())}; return
//   scalar_simplify_rules<ValueType>::instance().run(key, lhs, rhs);
// }

////template<typename ValueType>
////auto add_scalar_constants(expression_holder<scalar_expression<ValueType>>
/// const&lhs, / expression_holder<scalar_expression<ValueType>> const&rhs){ /
/// using type = scalar_constant<ValueType>; /  std::cout<<"const count lhs
///"<<lhs.data().use_count()<<std::endl; /  std::cout<<"const count rhs
///"<<lhs.data().use_count()<<std::endl; /  const auto value{lhs.template
/// get<type>()() + rhs.template get<type>()()}; /  return
/// make_expression<scalar_constant<ValueType>>(value);
////}

////auto temp = register_binary_simply<double, scalar_constant<double>,
/// scalar_constant<double>>( / [](expression_holder<scalar_expression<double>>
/// const& lhs, expression_holder<scalar_expression<double>> const& rhs){ /
/// using type = scalar_constant<double>; /      const auto value{lhs.template
/// get<type>()() + rhs.template get<type>()()}; /  return
/// make_expression<scalar_constant<double>>(value); /    });

///// constant + constant
// SYMTM_REGISTER_BINARY_SIMPLIFY(
//     double, scalar_constant, scalar_constant, add_constant_constant);

// SYMTM_REGISTER_BINARY_SIMPLIFY(
//     float, scalar_constant, scalar_constant, add_constant_constant);

// SYMTM_REGISTER_BINARY_SIMPLIFY(
//     int, scalar_constant, scalar_constant, add_constant_constant);

///// constant + negative
// SYMTM_REGISTER_BINARY_SIMPLIFY(
//     double, scalar_constant, scalar_negative, add_constant_negative);

// SYMTM_REGISTER_BINARY_SIMPLIFY(
//     float, scalar_constant, scalar_negative, add_constant_negative);

// SYMTM_REGISTER_BINARY_SIMPLIFY(
//     int, scalar_constant, scalar_negative, add_constant_negative);

///// negative + constant
// SYMTM_REGISTER_BINARY_SIMPLIFY(
//     double, scalar_negative, scalar_constant, add_negative_constant);

// SYMTM_REGISTER_BINARY_SIMPLIFY(
//     float, scalar_negative, scalar_constant, add_negative_constant);

// SYMTM_REGISTER_BINARY_SIMPLIFY(
//     int, scalar_negative, scalar_constant, add_negative_constant);

///// constant + negative(constant)
// SYMTM_REGISTER_SIMPLIFY(
//     double, add_constant_negative, scalar_constant, scalar_negative,
//     scalar_constant);

////}
////SYMTM_REGISTER_BINARY_SIMPLIFY(
////    float, scalar_constant, scalar_negative, scalar_constant,
/// scalar_rules::add_constant_negative);

////SYMTM_REGISTER_BINARY_SIMPLIFY(
////    int, scalar_constant, scalar_negative, scalar_constant,
/// scalar_rules::add_constant_negative);

template <typename ExprLHS, typename ExprRHS> class scalar_simplifier_add;

template <typename T>
auto binary_simplify_add(expression_holder<scalar_expression<T>> const &lhs,
                         expression_holder<scalar_expression<T>> const &rhs) {
  scalar_simplifier_add<expression_holder<scalar_expression<T>> const &,
                        expression_holder<scalar_expression<T>> const &>
      visitor(lhs, rhs);
  return std::visit(visitor, *lhs, *rhs);
}

// template<typename T>
// expression_holder<scalar_expression<T>>
// simplify_add(expression_holder<scalar_expression<T>> const& lhs,
// expression_holder<scalar_expression<T>> const& rhs) {
//   auto& registry = scalar_simplify_rules<T>::instance();
//   auto lhs_id = lhs->type_id();
//   auto rhs_id = rhs->type_id();

//  if (auto rule = registry.run(registry.get_key(lhs, rhs), lhs, rhs)) {
//    return (*rule)(lhs, rhs);
//  }

//  return build_add(lhs, rhs); // Fallback
//}

template <typename ExprLHS, typename ExprRHS> class scalar_simplifier_add {
public:
  using value_type = typename std::remove_reference_t<
      std::remove_const_t<ExprLHS>>::value_type;
  using expr_type = expression_holder<scalar_expression<value_type>>;

  scalar_simplifier_add(ExprLHS &&lhs, ExprRHS &&rhs)
      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs)) {}

  //  scalar_simplifier_add(ExprLHS &&lhs, ExprRHS rhs)
  //      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(rhs) {}

  auto operator()([[maybe_unused]] scalar_constant<value_type> &lhs,
                  [[maybe_unused]] scalar_constant<value_type> &rhs) {
    return visit(simplifier::add_base<ExprLHS, ExprRHS>(m_lhs, m_rhs), *m_lhs);
    // return std::visit(simplifier::constant_add<value_type>(m_lhs,m_rhs),
    // *m_rhs); auto
    // func{scalar_simplify_rules<value_type>::instance().lookup(lhs.get_id(),
    // rhs.get_id())}; return func(m_lhs, m_rhs); const auto value{lhs() +
    // rhs()}; return make_expression<scalar_constant<value_type>>(value);
  }

  auto operator()(scalar_constant<value_type> &lhs,
                  scalar_negative<value_type> &rhs) {
    if (is_same<scalar_constant<value_type>>(rhs.expr())) {
      const auto value{
          lhs() - rhs.expr().template get<scalar_constant<value_type>>()()};
      return make_expression<scalar_constant<value_type>>(value);
    }
    if (is_same<scalar_one<value_type>>(rhs.expr())) {
      const auto value{lhs() - static_cast<value_type>(1)};
      return make_expression<scalar_constant<value_type>>(value);
    }
    auto add_new{make_expression<scalar_add<value_type>>()};
    add_new.template get<scalar_add<value_type>>().push_back(
        std::forward<ExprLHS>(m_rhs));
    add_new.template get<scalar_add<value_type>>().set_coeff(
        std::forward<ExprRHS>(m_lhs));
    return std::move(add_new);
  }

  auto operator()(scalar_negative<value_type> &lhs,
                  scalar_constant<value_type> &rhs) {
    if (is_same<scalar_constant<value_type>>(lhs.expr())) {
      const auto value{
          rhs() - lhs.expr().template get<scalar_constant<value_type>>()()};
      return make_expression<scalar_constant<value_type>>(value);
    }
    if (is_same<scalar_one<value_type>>(lhs.expr())) {
      const auto value{rhs() - 1};
      return make_expression<scalar_constant<value_type>>(value);
    }
    auto add_new{make_expression<scalar_add<value_type>>()};
    add_new.template get<scalar_add<value_type>>().push_back(
        std::forward<ExprLHS>(m_lhs));
    add_new.template get<scalar_add<value_type>>().set_coeff(
        std::forward<ExprRHS>(m_rhs));
    return std::move(add_new);
  }

  // {1+x, 2} --> 3+x
  // {x+y, 2} --> 2+x+y
  auto operator()([[maybe_unused]] scalar_add<value_type> &lhs,
                  [[maybe_unused]] scalar_constant<value_type> &rhs) {
    // auto add_func{[&](auto &add) {}};
    //  we don't know if the scalar_constant inside the
    //  lhs expression is a l-value or not. therefor no
    //  move of data here.
    if constexpr (std::is_lvalue_reference_v<ExprLHS>) {
      // do copy
      auto add_expr{make_expression<scalar_add<value_type>>(lhs)};
      auto &add{add_expr.template get<scalar_add<value_type>>()};
      auto coeff{add.coeff() +
                 (m_negative ? -1 : 1) * std::forward<ExprRHS>(m_rhs)};
      lhs.set_coeff(std::move(coeff));
      return std::move(add_expr);
    } else {
      // use existing one
      auto coeff{lhs.coeff() + (m_negative ? -1 : 1) * m_rhs};
      lhs.set_coeff(std::move(coeff));
      return std::move(m_lhs);
    }
  }

  auto operator()([[maybe_unused]] scalar_constant<value_type> &lhs,
                  [[maybe_unused]] scalar_add<value_type> &rhs) {
    return std::forward<ExprRHS>(m_rhs) + std::forward<ExprLHS>(m_lhs);
  }

  /// {expr,1} --> 1+expr
  template <typename Scalar>
  auto operator()([[maybe_unused]] Scalar &lhs,
                  [[maybe_unused]] scalar_constant<value_type> &rhs) {
    auto add_new{make_expression<scalar_add<value_type>>()};
    add_new.template get<scalar_add<value_type>>().push_back(
        std::forward<ExprLHS>(m_lhs));
    add_new.template get<scalar_add<value_type>>().set_coeff(
        std::forward<ExprRHS>(m_rhs));
    return std::move(add_new);
  }

  template <typename Scalar>
  expr_type operator()([[maybe_unused]] scalar_constant<value_type> &rhs,
                       [[maybe_unused]] Scalar &lhs) {
    return std::forward<ExprRHS>(m_rhs) + std::forward<ExprLHS>(m_lhs);
  }

  /// x+x --> 2*x
  auto operator()(scalar<value_type> &lhs, scalar<value_type> &rhs) {
    if (&lhs == &rhs) {
      auto mul{make_expression<scalar_mul<value_type>>()};
      mul.template get<scalar_mul<value_type>>().set_coeff(
          make_expression<scalar_constant<value_type>>(2));
      mul.template get<scalar_mul<value_type>>().push_back(m_rhs);
      return std::move(mul);
    }

    return default_add();
  }

  /// x+negative(x) --> 0
  auto operator()(scalar<value_type> &lhs, scalar_negative<value_type> &rhs) {
    const auto &hash_rhs{rhs.expr().get().hash_value()};
    const auto &hash_lhs{lhs.hash_value()};
    if (hash_rhs == hash_lhs) {
      return get_scalar_zero<value_type>();
    }
    return default_add();
  }

  auto operator()(scalar_negative<value_type> &lhs,
                  scalar_negative<value_type> &rhs) {
    const auto &hash_rhs{rhs.expr().get().hash_value()};
    const auto &hash_lhs{lhs.expr().get().hash_value()};
    if (hash_rhs == hash_lhs) {
      auto expr{binary_simplify_add(lhs.expr(), rhs.expr())};
      return make_expression<scalar_negative<value_type>>(expr);
    }
    return default_add();
  }

  /// factor*scalar + scalar --> (factor+1)*scalar
  auto operator()(scalar_mul<value_type> &lhs, scalar<value_type> &rhs) {
    const auto &hash_rhs{rhs.hash_value()};
    const auto &hash_lhs{lhs.hash_value()};
    if (hash_rhs == hash_lhs) {
      auto expr{make_expression<scalar_mul<value_type>>(lhs)};
      auto &mul{expr.template get<scalar_mul<value_type>>()};
      mul.set_coeff(make_expression<scalar_constant<value_type>>(
          get_coefficient(lhs, 1.0) + 1.0));
      return std::move(expr);
    }
    return default_add();
  }

  /// scalar + factor*scalar --> factor*scalar + scalar
  auto operator()([[maybe_unused]] scalar<value_type> &lhs,
                  [[maybe_unused]] scalar_mul<value_type> &rhs) {
    return std::forward<ExprRHS>(m_rhs) + std::forward<ExprLHS>(m_lhs);
  }

  /// expr + expr --> 2*expr
  auto operator()(scalar_mul<value_type> &lhs, scalar_mul<value_type> &rhs) {
    const auto &hash_rhs{rhs.hash_value()};
    const auto &hash_lhs{lhs.hash_value()};
    if (hash_rhs == hash_lhs) {
      const auto fac_lhs{get_coefficient(lhs, 1.0)};
      const auto fac_rhs{get_coefficient(rhs, 1.0)};
      auto expr{make_expression<scalar_mul<value_type>>(lhs)};
      auto &mul{expr.template get<scalar_mul<value_type>>()};
      mul.set_coeff(
          make_expression<scalar_constant<value_type>>(fac_lhs + fac_rhs));
      return std::move(expr);
    }
    return default_add();
  }

  auto operator()(scalar_add<value_type> &lhs, scalar<value_type> &rhs) {
    /// do a deep copy of data
    auto expr_add{make_expression<scalar_add<value_type>>(lhs)};
    auto &add{expr_add.template get<scalar_add<value_type>>()};
    /// check if sub_exp == expr_rhs for sub_exp \in expr_lhs
    auto pos{lhs.hash_map().find(rhs.hash_value())};
    if (pos != lhs.hash_map().end()) {
      auto expr{std::visit(
          scalar_simplifier_add<expr_type &, expr_type &>(pos->second, m_rhs),
          *pos->second, *m_rhs)};
      add.hash_map().erase(rhs.hash_value());
      add.push_back(expr);
      return expr_add;
    }
    /// no equal expr or sub_expr
    add.push_back(m_rhs);
    return expr_add;
  }

  // merge two expression
  auto operator()(scalar_add<value_type> &lhs, scalar_add<value_type> &rhs) {
    auto expr{make_expression<scalar_add<value_type>>()};
    auto &add{expr.template get<scalar_add<value_type>>()};
    merge(lhs, rhs, add, []() {});
    return expr;
  }

  /// (factor_lhs+expr_lhs) + (factor_rhs+expr_rhs)
  /// check if expr_rhs == expr_lhs --> (factor_lhs+factor_rhs)*(2*expr_lhs)
  /// check if sub_exp == expr_rhs for sub_exp \in expr_lhs
  template <typename Scalar>
  auto operator()(scalar_add<value_type> &lhs, Scalar &rhs) {
    return add_to_n_ary(lhs, rhs);
  }

  auto operator()([[maybe_unused]] scalar_add<value_type> &lhs,
                  scalar_negative<value_type> &rhs) {
    scalar_simplifier_add<expr_type &, expr_type &> visitor(m_lhs, m_rhs);
    visitor.m_negative = true;
    return std::visit(visitor, *m_lhs, *rhs.expr());
  }

  template <typename ScalarExpr>
  auto operator()([[maybe_unused]] ScalarExpr &lhs,
                  [[maybe_unused]] scalar_zero<value_type> &rhs) {
    return m_lhs;
  }

  template <typename ScalarExpr>
  auto operator()([[maybe_unused]] scalar_zero<value_type> &lhs,
                  [[maybe_unused]] ScalarExpr &rhs) {
    return m_rhs;
  }

  auto operator()([[maybe_unused]] scalar_zero<value_type> &lhs,
                  [[maybe_unused]] scalar_zero<value_type> &rhs) {
    return m_rhs;
  }

  auto operator()([[maybe_unused]] scalar_constant<value_type> &lhs,
                  [[maybe_unused]] scalar_zero<value_type> &rhs) {
    return m_lhs;
  }

  auto operator()([[maybe_unused]] scalar_zero<value_type> &lhs,
                  [[maybe_unused]] scalar_constant<value_type> &rhs) {
    return m_rhs;
  }

  auto operator()([[maybe_unused]] scalar_add<value_type> &lhs,
                  [[maybe_unused]] scalar_zero<value_type> &rhs) {
    return m_lhs;
  }

  auto operator()([[maybe_unused]] scalar_zero<value_type> &lhs,
                  [[maybe_unused]] scalar_add<value_type> &rhs) {
    return m_rhs;
  }

  template <typename ScalarLHS, typename ScalarRHS>
  auto operator()([[maybe_unused]] ScalarLHS &lhs,
                  [[maybe_unused]] ScalarRHS &rhs) {
    // constexpr auto expr_lhs{std::detect<has_expr<ScalarLHS>>};
    // constexpr auto expr_rhs{std::detect<has_expr<ScalarRHS>>};
    // if constexpr (expr_lhs && expr_lhs){}
    // if constexpr (expr_lhs && !expr_lhs){}
    // if constexpr (!expr_lhs && expr_lhs){}

    if (lhs.hash_value() == rhs.hash_value()) {
      return combine();
    }

    return default_add();
  }

  bool m_negative{false};

private:
  template <typename Scalar>
  auto add_to_n_ary(scalar_add<value_type> &lhs, Scalar &rhs) {
    /// check if expr_rhs == expr_lhs --> (factor_lhs+factor_rhs)*expr_lhs
    if (lhs.hash_value() == rhs.hash_value()) {
      auto expr{make_expression<scalar_mul<value_type>>()};
      auto &mul{expr.template get<scalar_mul<value_type>>()};
      mul.push_back(make_expression<Scalar>(rhs));
      mul.set_coeff(make_expression<scalar_constant<value_type>>(2));
      const auto coeff_lhs{get_coefficient(lhs, 0.0)};
      const auto coeff_rhs{get_coefficient(rhs, 0.0)};
      if (coeff_lhs != 0 || coeff_rhs != 0) {
        auto expr_add{make_expression<scalar_add<value_type>>()};
        expr_add.template get<scalar_add<value_type>>().set_coeff(
            make_expression<scalar_constant<value_type>>(coeff_rhs +
                                                         coeff_lhs));
        expr_add.template get<scalar_add<value_type>>().push_back(
            std::move(expr));
        return std::move(expr_add);
      }
      return std::move(expr);
    }

    /// check if sub_exp == expr_rhs for sub_exp \in expr_lhs
    auto pos{lhs.hash_map().find(rhs.hash_value())};
    if (pos != lhs.hash_map().end()) {
      auto expr{std::visit(
          scalar_simplifier_add<expr_type &, expr_type &>(pos->second, m_rhs),
          *pos->second, *m_rhs)};
      if (expr.is_valid()) {
        auto expr_add{make_expression<scalar_add<value_type>>(lhs)};
        auto &add{expr_add.template get<scalar_add<value_type>>()};
        add.hash_map().erase(rhs.hash_value());
        add.push_back(std::move(expr));
        return std::move(expr_add);
      }
    }

    /// no equal expr or sub_expr
    lhs.push_back(m_rhs);
    return m_lhs;
  }

  auto default_add() {
    auto add_new{make_expression<scalar_add<value_type>>()};
    add_new.template get<scalar_add<value_type>>().push_back(
        std::forward<ExprLHS>(m_lhs));
    add_new.template get<scalar_add<value_type>>().push_back(
        std::forward<ExprRHS>(m_rhs));
    return std::move(add_new);
  }

  auto combine() {
    auto mul_new{make_expression<scalar_mul<value_type>>()};
    mul_new.template get<scalar_mul<value_type>>().push_back(
        std::forward<ExprLHS>(m_lhs));
    mul_new.template get<scalar_mul<value_type>>().set_coeff(
        make_expression<scalar_constant<value_type>>(2));
    return std::move(mul_new);
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

  ExprLHS &&m_lhs;
  ExprRHS &&m_rhs;
};

// template <typename ExprLHS, typename ExprRHS> class
// scalar_simplifier_add_main { public:
//   using value_type = typename std::remove_reference_t<
//       std::remove_const_t<ExprLHS>>::value_type;
//   using expr_type = expression_holder<scalar_expression<value_type>>;

//  scalar_simplifier_add_main(ExprLHS &&lhs, ExprRHS &&rhs)
//      : m_lhs(std::forward<ExprLHS>(lhs)), m_rhs(std::forward<ExprRHS>(rhs))
//      {}

//  template<typename LHS, typename RHS>
//  expr_type operator()([[maybe_unused]]LHS &lhs, [[maybe_unused]]RHS &rhs) {
//    //scalar_simplifier_add<ExprLHS, ExprRHS>
//    visitor(std::forward<ExprLHS>(m_lhs), std::forward<ExprRHS>(m_rhs));
//    return std::visit(
//        [&](auto&, auto&){
//          return default_add();
//          //return expr_type();/*default_add();*/
//        }, lhs.data(), rhs.data());
//    //return std::visit(overload{visitor, [&visitor](auto&, auto&){return
//    visitor.default_add();}}, lhs.data(), rhs.data());
//  }

//  auto default_add() {
//    auto add_new{make_expression<scalar_add<value_type>>()};
//    add_new.template get<scalar_add<value_type>>().push_back(
//        std::forward<ExprLHS>(m_lhs));
//    add_new.template get<scalar_add<value_type>>().push_back(
//        std::forward<ExprRHS>(m_rhs));
//    return std::move(add_new);
//  }

//  template <typename _Expr, typename _ValueType>
//  constexpr auto get_coefficient(_Expr const &expr, _ValueType const &value) {
//    if constexpr (is_detected_v<has_coefficient, _Expr>) {
//      auto func{[&](auto const &coeff) {
//        return coeff.is_valid() ? coeff.template
//        get<scalar_constant<value_type>>()()
//                                : value;
//      }};
//      return func(expr.coeff());
//    }
//    return value;
//  }

// private:
//   ExprLHS &&m_lhs;
//   ExprRHS &&m_rhs;
// };

} // namespace numsim::cas
#endif // SCALAR_SIMPLIFIER_ADD_H
