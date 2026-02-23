#ifndef TENSOR_TO_SCALAR_SIMPLIFIER_MUL_H
#define TENSOR_TO_SCALAR_SIMPLIFIER_MUL_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/simplifier/simplifier_mul.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar_div.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar_mul.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_domain_traits.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

using tensor_to_scalar_traits = domain_traits<tensor_to_scalar_expression>;

namespace tensor_to_scalar_detail {
namespace simplifier {

template <typename Derived>
class mul_default
    : public tensor_to_scalar_visitor_return_expr_t,
      public detail::mul_dispatch<tensor_to_scalar_traits, Derived> {
  using algo = detail::mul_dispatch<tensor_to_scalar_traits, Derived>;

public:
  using expr_holder_t = typename algo::expr_holder_t;
  using algo::algo;
  using algo::dispatch;
  using algo::get_default;

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

  using algo::m_lhs;
  using algo::m_rhs;
};

class constant_mul final : public mul_default<constant_mul> {
public:
  using expr_holder_t = expression_holder<tensor_to_scalar_expression>;
  using base = mul_default<constant_mul>;
  using base::operator();
  using base::dispatch;
  using base::m_rhs;

  constant_mul(expr_holder_t lhs, expr_holder_t rhs);

  // scalar_wrapper * scalar_wrapper → numeric multiply
  expr_holder_t dispatch(tensor_to_scalar_scalar_wrapper const &rhs);

  // scalar_wrapper * mul → merge coefficient
  expr_holder_t dispatch(tensor_to_scalar_mul const &rhs);

  template <typename ExprType>
  expr_holder_t dispatch([[maybe_unused]] ExprType const &) {
    if (lhs_val && *lhs_val == scalar_number{1}) {
      return std::move(m_rhs);
    }
    return base::get_default();
  }

private:
  std::optional<scalar_number> lhs_val;
};

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

  [[nodiscard]] expr_holder_t dispatch(tensor_to_scalar_zero const &);

  [[nodiscard]] expr_holder_t dispatch(tensor_to_scalar_one const &);

  [[nodiscard]] expr_holder_t dispatch(tensor_to_scalar_negative const &);

  [[nodiscard]] expr_holder_t dispatch(tensor_to_scalar_scalar_wrapper const &);

  template <typename Type> [[nodiscard]] expr_holder_t dispatch(Type const &) {
    auto &_rhs{m_rhs.template get<tensor_to_scalar_visitable_t>()};
    mul_default<void> visitor(std::move(m_lhs), std::move(m_rhs));
    return _rhs.accept(visitor);
  }

  expr_holder_t m_lhs;
  expr_holder_t m_rhs;
};

} // namespace simplifier
} // namespace tensor_to_scalar_detail
} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_SIMPLIFIER_MUL_H
