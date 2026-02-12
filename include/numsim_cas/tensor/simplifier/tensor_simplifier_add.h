#ifndef TENSOR_SIMPLIFIER_ADD_H
#define TENSOR_SIMPLIFIER_ADD_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {
namespace simplifier {
namespace tensor_detail {

template <typename Derived>
class add_default : public tensor_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<tensor_expression>;

  add_default(expr_holder_t lhs, expr_holder_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

  [[nodiscard]] auto get_default() {
    if (m_lhs.get().hash_value() == m_rhs.get().hash_value()) {
      auto constant{make_expression<scalar_constant>(2)};
      return make_expression<tensor_scalar_mul>(std::move(constant),
                                                std::move(m_rhs));
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

  [[nodiscard]] expr_holder_t dispatch(tensor_negative const &expr) {
    if (m_lhs.get().hash_value() == expr.expr().get().hash_value()) {
      return make_expression<tensor_zero>(expr.dim(), expr.rank());
    }
    return get_default();
  }

  template <typename Expr> [[nodiscard]] expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

  [[nodiscard]] expr_holder_t dispatch(tensor_zero const &) {
    return std::move(m_lhs);
  }

  //  expr_holder_t dispatch(tensor_zero const&){
  //    return m_lhs;
  //  }

  //  template <typename _Expr, typename _ValueType>
  //  auto get_coefficient(_Expr const &expr, _ValueType const &value)
  //  {
  //    if (is_detected_v<has_coefficient, _Expr>) {
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
#define NUMSIM_ADD_OVR(T)                                                      \
  expr_holder_t operator()(T const &n) override {                              \
    if constexpr (std::is_void_v<Derived>) {                                   \
      return dispatch(n);                                                      \
    } else {                                                                   \
      return static_cast<Derived *>(this)->dispatch(n);                        \
    }                                                                          \
  }
  NUMSIM_CAS_TENSOR_NODE_LIST(NUMSIM_ADD_OVR, NUMSIM_ADD_OVR)
#undef NUMSIM_ADD_OVR

protected:
  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

// template <typename ExprLHS, typename ExprRHS>
// class constant_add final : public add_default{
// public:
//   using value_type = typename std::remove_reference_t<
//       std::remove_const_t<ExprLHS>>::value_type;
//   using expr_holder_t = expression_holder<scalar_expression>;
//   using base = add_default;
//   using base::dispatch;
//   using base::get_coefficient;
//   constant_add(ExprLHS lhs, ExprRHS
//   rhs):base(std::move(lhs),std::move(rhs)),
//                                            lhs(base::m_lhs.template
//                                            get<tensor_constant>()){}

//  expr_holder_t dispatch(tensor_constant const&
//  rhs){
//    const auto value{lhs() + rhs()};
//    return make_expression<tensor_constant>(value);
//  }

////  expr_holder_t
/// dispatch([[maybe_unused]]tensor_add const& rhs){ /    auto
/// add_expr{make_expression<scalar_add>(rhs)}; /    auto
///&add{add_expr.template get<scalar_add>()}; /    auto
/// coeff{add.coeff() + base::m_lhs}; /    add.set_coeff(std::move(coeff)); /
/// return std::move(add_expr); /  }

// private:
//   tensor_constant const& lhs;
// };

class n_ary_add final : public add_default<n_ary_add> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = add_default<n_ary_add>;
  using base::dispatch;
  using base::get_default;
  // using base::get_coefficient;

  n_ary_add(expr_holder_t lhs, expr_holder_t rhs);

  //  expr_holder_t
  //  dispatch([[maybe_unused]]tensor_constant const& rhs){
  //    auto add_expr{make_expression<tensor_add>(lhs)};
  //    auto &add{add_expr.template get<tensor_add>()};
  //    auto coeff{add.coeff() + m_rhs};
  //    add.set_coeff(std::move(coeff));
  //    return std::move(add_expr);
  //  }

  //  expr_holder_t
  //  dispatch([[maybe_unused]]tensor_one const& ){
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
  [[nodiscard]] auto dispatch([[maybe_unused]] Expr const &rhs);

  // merge two expression
  [[nodiscard]] auto dispatch(tensor_add const &rhs);

  [[nodiscard]] auto dispatch(tensor_negative const &rhs);

protected:
  using base::m_lhs;
  using base::m_rhs;
  tensor_add const &lhs;
};

// template<typename T>
// class n_ary_mul_add final : public add_default<T>{
// public:
//   using value_type = T;
//   using expr_holder_t = expression_holder<scalar_expression>;
//   using base = add_default<T>;
//   using base::dispatch;
//   using base::get_default;
//   using base::get_coefficient;

//  n_ary_mul_add(expr_holder_t lhs, expr_holder_t
//  rhs):base(lhs,rhs),lhs{base::m_lhs.template get<scalar_mul>()}
//  {}

//  auto dispatch(scalar const&rhs) {
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
//  auto dispatch(scalar_mul const&rhs) {
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

class symbol_add final : public add_default<symbol_add> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = add_default<symbol_add>;
  using base::dispatch;
  using base::get_default;
  //  using base::get_coefficient;

  symbol_add(expr_holder_t lhs, expr_holder_t rhs);

  /// x+x --> 2*x
  [[nodiscard]] expr_holder_t dispatch(tensor const &rhs);

  //  expr_holder_t  dispatch(scalar_mul const&rhs) {
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
  //  expr_holder_t dispatch(scalar_constant
  //  const&rhs) {
  //  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor const &lhs;
};

class tensor_scalar_mul_add final : public add_default<tensor_scalar_mul_add> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = add_default<tensor_scalar_mul_add>;
  using base::dispatch;
  using base::get_default;

  tensor_scalar_mul_add(expr_holder_t lhs, expr_holder_t rhs);

  // scalar_expr * lhs + rhs
  template <typename Expr>
  [[nodiscard]] expr_holder_t dispatch(Expr const &rhs);

  // scalar_expr_lhs * lhs + scalar_expr_rhs * rhs
  [[nodiscard]] expr_holder_t dispatch(tensor_scalar_mul const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_scalar_mul const &lhs;
};

class add_negative final : public add_default<add_negative> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = add_default<add_negative>;
  using base::dispatch;
  // using base::get_coefficient;
  using base::get_default;

  add_negative(expr_holder_t lhs, expr_holder_t rhs);

  // (-lhs) + (-rhs) --> -(lhs+rhs)
  [[nodiscard]] expr_holder_t dispatch(tensor_negative const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_negative const &lhs;
};

class add_base final : public tensor_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<tensor_expression>;

  add_base(expr_holder_t lhs, expr_holder_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

protected:
#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_TENSOR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  [[nodiscard]] expr_holder_t dispatch(tensor_zero const &);

  [[nodiscard]] expr_holder_t dispatch(tensor_add const &);

  [[nodiscard]] expr_holder_t dispatch(tensor_scalar_mul const &);

  [[nodiscard]] expr_holder_t dispatch(tensor_negative const &);

  [[nodiscard]] expr_holder_t dispatch(tensor const &);

  template <typename Type> [[nodiscard]] expr_holder_t dispatch(Type const &);

  // template <typename Type>
  // [[nodiscard]] expr_holder_t dispatch(Type const &) {
  //   auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
  //   add_default<void> visitor(std::move(m_lhs), std::move(m_rhs));
  //   return _rhs.accept(visitor);
  // }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

} // namespace tensor_detail
} // namespace simplifier

} // namespace numsim::cas
#endif // TENSOR_SIMPLIFIER_ADD_H
