#ifndef TENSOR_SIMPLIFIER_MUL_H
#define TENSOR_SIMPLIFIER_MUL_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_std.h>

namespace numsim::cas {
namespace tensor_detail {
namespace simplifier {

template <typename Derived>
class mul_default : public tensor_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<tensor_expression>;

  mul_default(expr_holder_t lhs, expr_holder_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

  //    //if(rhs_constant){
  //    //  add.set_coeff(m_rhs);
  //    //}else{
  //      add.push_back(m_rhs);
  //    //}
  //    return std::std::move(add_new);
  //  }

  // expr * 0 --> 0
  expr_holder_t dispatch(tensor_zero const &) { return m_rhs; }

  expr_holder_t dispatch(kronecker_delta const &) { return m_lhs; }

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

  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

protected:
  expr_holder_t get_default() {
    if (m_lhs.get().hash_value() == m_rhs.get().hash_value()) {
      return pow(std::move(m_lhs), 2);
    }
    // const auto lhs_constant{is_same<tensor_constant>(m_lhs)};
    // const auto rhs_constant{is_same<tensor_constant>(m_rhs)};
    auto mul_new{
        make_expression<tensor_mul>(m_lhs.get().dim(), m_rhs.get().rank())};
    auto &add{mul_new.template get<tensor_mul>()};
    // if(lhs_constant){
    //   add.set_coeff(m_lhs);
    // }else{
    add.push_back(m_lhs);
    add.push_back(m_rhs);
    //}
    return mul_new;
  }

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

class tensor_pow_mul final : public mul_default<tensor_pow_mul> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = mul_default<tensor_pow_mul>;
  using base::dispatch;
  using base::get_default;

  tensor_pow_mul(expr_holder_t lhs, expr_holder_t rhs);

  expr_holder_t dispatch([[maybe_unused]] tensor const &rhs);

  expr_holder_t dispatch([[maybe_unused]] tensor_pow const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_pow const &lhs;
};

class kronecker_delta_mul final : public mul_default<kronecker_delta_mul> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = mul_default<kronecker_delta_mul>;
  using base::dispatch;
  using base::get_default;

  kronecker_delta_mul(expr_holder_t lhs, expr_holder_t rhs);

  // I_ij*expr_jkmnop.... --> expr_ikmnop....
  template <typename Expr>
  expr_holder_t dispatch([[maybe_unused]] Expr const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  kronecker_delta const &lhs;
};

class symbol_mul final : public mul_default<symbol_mul> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = mul_default<symbol_mul>;
  using base::dispatch;
  using base::get_default;

  symbol_mul(expr_holder_t lhs, expr_holder_t rhs);

  /// X*X --> pow(X,2)
  expr_holder_t dispatch(tensor const &rhs);

  /// X * pow(X,expr) --> pow(X,expr+1)
  expr_holder_t dispatch(tensor_pow const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor const &lhs;
};

class n_ary_mul final : public mul_default<n_ary_mul> {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  using base = mul_default<n_ary_mul>;
  using base::dispatch;

  n_ary_mul(expr_holder_t lhs, expr_holder_t rhs);

  // check if last element == rhs; if not just pushback element
  template <typename Expr>
  expr_holder_t dispatch([[maybe_unused]] Expr const &rhs);

  // merge to tensor_mul objects
  expr_holder_t dispatch([[maybe_unused]] tensor_mul const &rhs);

private:
  using base::m_lhs;
  using base::m_rhs;
  tensor_mul const &lhs;
};

class mul_base final : public tensor_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<tensor_expression>;
  mul_base(expr_holder_t lhs, expr_holder_t rhs);

protected:
#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_TENSOR_NODE_LIST(NUMSIM_ADD_OVR_FIRST, NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  expr_holder_t dispatch(tensor const &);

  expr_holder_t dispatch(tensor_zero const &);

  expr_holder_t dispatch(tensor_pow const &);

  expr_holder_t dispatch(kronecker_delta const &);

  expr_holder_t dispatch(tensor_mul const &);

  template <typename Expr> expr_holder_t dispatch(Expr const &) {
    auto &_rhs{m_rhs.template get<tensor_visitable_t>()};
    mul_default<void> visitor(std::move(m_lhs), std::move(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

} // namespace simplifier
} // namespace tensor_detail
} // namespace numsim::cas

#endif // TENSOR_SIMPLIFIER_MUL_H
