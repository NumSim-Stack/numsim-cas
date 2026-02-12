#ifndef TENSOR_TO_SCALAR_SIMPLIFIER_MUL_H
#define TENSOR_TO_SCALAR_SIMPLIFIER_MUL_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_div.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_mul.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {
namespace tensor_to_scalar_detail {
namespace simplifier {

template <typename Derived>
class mul_default : public tensor_to_scalar_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<tensor_to_scalar_expression>;

  mul_default(expr_holder_t lhs, expr_holder_t rhs)
      : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

  [[nodiscard]] expr_holder_t get_default() {
    if (m_lhs == m_rhs) {
      return get_default_same();
    }
    return get_default_imp();
  }

  template <typename Expr> [[nodiscard]] expr_holder_t dispatch(Expr const &) {
    return get_default();
  }

  // // tensor_to_scalar * tensor_to_scalar_with_scalar_mul -->
  // // tensor_to_scalar_with_scalar_mul
  // [[nodiscard]] expr_holder_t
  // dispatch(tensor_to_scalar_with_scalar_mul const &rhs) noexcept {
  //   return make_expression<tensor_to_scalar_with_scalar_mul>(
  //       rhs.expr_lhs(), rhs.expr_rhs() * m_lhs);
  // }

protected:
#define NUMSIM_ADD_OVR(T)                                                      \
  expr_holder_t operator()(T const &n) override {                              \
    if constexpr (std::is_void_v<Derived>) {                                   \
      return dispatch(n);                                                      \
    } else {                                                                   \
      return static_cast<Derived *>(this)->dispatch(n);                        \
    }                                                                          \
  }
  NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_ADD_OVR, NUMSIM_ADD_OVR)
#undef NUMSIM_ADD_OVR

  [[nodiscard]] expr_holder_t get_default_same() {
    auto coeff{make_expression<tensor_to_scalar_scalar_wrapper>(
        make_expression<scalar_constant>(2))};
    return make_expression<tensor_to_scalar_pow>(m_rhs, std::move(coeff));
  }

  [[nodiscard]] expr_holder_t get_default_imp() {
    auto mul_new{make_expression<tensor_to_scalar_mul>()};
    auto &mul{mul_new.template get<tensor_to_scalar_mul>()};
    mul.push_back(m_lhs);
    mul.push_back(m_rhs);
    return mul_new;
  }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

// class wrapper_tensor_to_scalar_mul_mul final
//     : public mul_default<wrapper_tensor_to_scalar_mul_mul> {
// public:
//   using expr_holder_t = expression_holder<tensor_to_scalar_expression>;
//   using base = mul_default<wrapper_tensor_to_scalar_mul_mul>;

//   wrapper_tensor_to_scalar_mul_mul(expr_holder_t lhs, expr_holder_t rhs);

//   // tensor_to_scalar_with_scalar_mul * tensor_to_scalar_with_scalar_mul -->
//   // tensor_to_scalar_with_scalar_mul
//   [[nodiscard]] expr_holder_t
//   dispatch(tensor_to_scalar_with_scalar_mul const &rhs);

//   // tensor_to_scalar_with_scalar_mul * tensor_to_scalar -->
//   // tensor_to_scalar_with_scalar_mul
//   template <typename Expr>
//   [[nodiscard]] expr_holder_t dispatch(Expr const &);

// private:
//   using base::m_lhs;
//   using base::m_rhs;
//   tensor_to_scalar_with_scalar_mul const &lhs;
// };

class mul_base final : public tensor_to_scalar_visitor_return_expr_t {
public:
  using expr_holder_t = expression_holder<tensor_to_scalar_expression>;

  mul_base(expr_holder_t lhs, expr_holder_t rhs);

protected:
#define NUMSIM_ADD_OVR_FIRST(T)                                                \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
#define NUMSIM_ADD_OVR_NEXT(T)                                                 \
  expr_holder_t operator()(T const &lhs) override { return dispatch(lhs); }
  NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_ADD_OVR_FIRST,
                                        NUMSIM_ADD_OVR_NEXT)
#undef NUMSIM_ADD_OVR_FIRST
#undef NUMSIM_ADD_OVR_NEXT

  [[nodiscard]] expr_holder_t dispatch(tensor_to_scalar_mul const &);

  template <typename Type> [[nodiscard]] expr_holder_t dispatch(Type const &);

  // // tensor_scalar_with_scalar_mul * tensor_scalar -->
  // // tensor_scalar_with_scalar_mul
  // [[nodiscard]] expr_holder_t
  // dispatch(tensor_to_scalar_with_scalar_mul const &);

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_SIMPLIFIER_MUL_H
