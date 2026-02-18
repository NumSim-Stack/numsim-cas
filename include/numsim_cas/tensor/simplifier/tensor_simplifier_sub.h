#ifndef TENSOR_SIMPLIFIER_SUB_H
#define TENSOR_SIMPLIFIER_SUB_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_std.h>

namespace numsim::cas {
namespace tensor_detail {
namespace simplifier {

template <typename Derived>
class sub_default : public tensor_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<tensor_expression>;

  sub_default(expr_holder_t lhs, expr_holder_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

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
    add.push_back(-m_rhs);
    //    }
    return std::move(add_new);
  }

  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

  //         // expr - 0 --> expr
  //  expr_holder_t dispatch(tensor_zero const&){
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
  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

class negative_sub final : public sub_default<negative_sub> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = sub_default<negative_sub>;
  using base::dispatch;
  // using base::get_coefficient;

  negative_sub(expr_holder_t lhs, expr_holder_t rhs);

  //  template<typename Expr>
  //  expr_holder_t
  //  dispatch([[maybe_unused]]scalar_add const& rhs){
  //    return make_expression<scalar_negative>(m_lhs + m_rhs);
  //  }

  //         //-expr - rhs
  //  expr_holder_t dispatch(scalar_constant const&
  //  rhs){
  //    const auto value{-lhs() - rhs()};
  //    return make_expression<scalar_constant>(value);
  //  }

  //  //-expr - (constant + x)
  //  //-expr - constant - x
  //  //-(expr + constant + x)
  //  expr_holder_t
  //  dispatch([[maybe_unused]]scalar_add const& rhs){
  //    auto add_expr{make_expression<scalar_add>(rhs)};
  //    auto &add{add_expr.template get<scalar_add>()};
  //    auto coeff{base::m_lhs + add.coeff()};
  //    add.set_coeff(std::move(coeff));
  //    return
  //    make_expression<scalar_negative>(std::move(add_expr));
  //  }

  //  //
  //  expr_holder_t dispatch(scalar_one const& ){
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
//   using expr_holder_t = expression_holder<scalar_expression>;
//   using base = sub_default<T>;
//   using base::dispatch;
//   using base::get_coefficient;

//  constant_sub(expr_holder_t lhs, expr_holder_t
//  rhs):base(lhs,rhs),lhs{base::m_lhs.template
//  get<tensor_constant>()}
//  {}

//         //lhs - rhs
//  expr_holder_t dispatch(scalar_constant const&
//  rhs){
//    const auto value{lhs() - rhs()};
//    return make_expression<scalar_constant>(value);
//  }

//         // constant_lhs - (constant + x)
//         // constant_lhs - constant - x
//  expr_holder_t dispatch([[maybe_unused]]scalar_add
//  const& rhs){
//    assert(true);
//    auto add_expr{make_expression<scalar_add>(rhs)};
//    auto &add{add_expr.template get<scalar_add>()};
//    auto coeff{base::m_lhs - add.coeff()};
//    add.set_coeff(std::move(coeff));
//    return std::move(add_expr);
//  }

//  expr_holder_t dispatch(scalar_one const& ){
//    const auto value{lhs() - 1};
//    return make_expression<scalar_constant>(value);
//  }

// private:
//   scalar_constant const& lhs;
// };

class n_ary_sub final : public sub_default<n_ary_sub> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = sub_default<n_ary_sub>;
  using base::dispatch;
  // using base::get_coefficient;

  n_ary_sub(expr_holder_t lhs, expr_holder_t rhs);

  //  expr_holder_t
  //  dispatch([[maybe_unused]]scalar_constant const& rhs){
  //    auto add_expr{make_expression<scalar_add>(lhs)};
  //    auto &add{add_expr.template get<scalar_add>()};
  //    auto coeff{add.coeff() + m_rhs};
  //    add.set_coeff(std::move(coeff));
  //    return std::move(add_expr);
  //  }

  //  expr_holder_t
  //  dispatch([[maybe_unused]]scalar_one const& ){
  //    auto add_expr{make_expression<scalar_add>(lhs)};
  //    auto &add{add_expr.template get<scalar_add>()};
  //    auto
  //    coeff{make_expression<scalar_constant>(get_coefficient(add,
  //    0.0) + static_cast(1))};
  //    //auto coeff{add.coeff() + m_rhs};
  //    add.set_coeff(std::move(coeff));
  //    return add_expr;
  //  }

  //  auto dispatch([[maybe_unused]]scalar const&rhs) {
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
  //  auto dispatch(scalar_add const&rhs) {
  //    auto expr{make_expression<scalar_add>()};
  //    auto& add{expr.template get<scalar_add>()};
  //    add.set_coeff(lhs.coeff() + rhs.coeff());
  //    std::set<expr_holder_t, expression_comparator> used_expr;
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
//   using expr_holder_t = expression_holder<tensor_expression>;
//   using base = sub_default<T>;
//   using base::dispatch;
//   using base::get_default;
//   //using base::get_coefficient;

//  n_ary_mul_sub(expr_holder_t lhs, expr_holder_t
//  rhs):base(lhs,rhs),lhs{base::m_lhs.template get<tensor_mul>()}
//  {}

//  //  auto dispatch(scalar const&rhs) {
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
//  //  auto dispatch(scalar_mul const&rhs) {
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

class symbol_sub final : public sub_default<symbol_sub> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = sub_default<symbol_sub>;
  using base::dispatch;
  using base::get_default;
  // using base::get_coefficient;

  symbol_sub(expr_holder_t lhs, expr_holder_t rhs);

  /// x-x --> 0
  expr_holder_t dispatch(tensor const &rhs);

  //         //x - 3*x --> -(2*x)
  //  expr_holder_t  dispatch(scalar_mul const&rhs) {
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

  //  expr_holder_t dispatch(scalar_constant
  //  const&rhs) {
  //  }

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor const &lhs;
};

class sub_base final : public tensor_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<tensor_expression>;

  sub_base(expr_holder_t lhs, expr_holder_t rhs);

protected:
#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_TENSOR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  //  expr_holder_t dispatch(scalar_constant const&){
  //    return visit(constant_sub(m_lhs,m_rhs), *m_rhs);
  //  }

  expr_holder_t dispatch(tensor_add const &);

  //  expr_holder_t dispatch(scalar_mul const&){
  //    return visit(n_ary_mul_sub(m_lhs,m_rhs), *m_rhs);
  //  }

  expr_holder_t dispatch(tensor const &);

  // 0 - expr
  expr_holder_t dispatch(tensor_zero const &);

  // - expr_lhs - expr_rhs --> -(expr_lhs+expr_rhs)
  expr_holder_t dispatch(tensor_negative const &lhs);

  template <typename Type> expr_holder_t dispatch(Type const &) {
    auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
    sub_default<void> visitor(std::move(m_lhs), std::move(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

} // namespace simplifier
} // namespace tensor_detail
} // namespace numsim::cas

#endif // TENSOR_SIMPLIFIER_SUB_H
