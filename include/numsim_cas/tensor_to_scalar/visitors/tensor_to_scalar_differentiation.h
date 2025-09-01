#ifndef TENSOR_TO_SCALAR_DIFFERENTIATION_H
#define TENSOR_TO_SCALAR_DIFFERENTIATION_H

#include "../../numsim_cas_type_traits.h"
#include "../../operators.h"
#include "../../printer_base.h"
#include "../../scalar/visitors/scalar_printer.h"
#include "../../tensor/tensor_functions_fwd.h"
#include "../../tensor/visitors/tensor_printer.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

namespace numsim::cas {

template <typename ValueType> class tensor_to_scalar_differentiation {
public:
  using value_type = ValueType;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;
  using result_expr_type = expression_holder<tensor_expression<value_type>>;

  explicit tensor_to_scalar_differentiation(result_expr_type const &arg)
      : m_arg(arg) {
    I = make_expression<kronecker_delta<ValueType>>(arg.get().dim());
  }
  tensor_to_scalar_differentiation(tensor_to_scalar_differentiation const &) =
      delete;
  tensor_to_scalar_differentiation(tensor_to_scalar_differentiation &&) =
      delete;
  const tensor_to_scalar_differentiation &
  operator=(tensor_to_scalar_differentiation const &) = delete;

  auto apply(expr_type const &expr) {
    if (expr.is_valid()) {
      m_expr = expr;
      std::visit([this](auto &&arg) { (*this)(arg); }, *expr);
    }
    return m_result;
  }

  // trace(expr) = I:expr = expr_ii
  // dtrace(expr)/dX = dtrace(expr)/dexpr:dexpr/dX
  //                 = I:dexpr
  void operator()([[maybe_unused]] tensor_trace<ValueType> const &visitable) {
    auto result{diff(visitable.expr(), m_arg)};
    if (result.is_valid()) {
      m_result = inner_product(I, sequence{1, 2}, result, sequence{2, 4});
    }
  }

  // expr_ijkl...*expr_ijkl...
  void operator()([[maybe_unused]] tensor_dot<ValueType> const &visitable) {}

  // dnorm(expr)/dX = dsqrt(dot(expr,expr))/dX
  //                = dnorm(expr)/dexpr : dexpr/dX
  //                = expr/norm(expr) : dexpr/dX
  void operator()([[maybe_unused]] tensor_norm<ValueType> const &visitable) {
    auto result{diff(visitable.expr(), m_arg)};
    if (result.is_valid()) {
      m_result = inner_product(visitable.expr(), sequence{1, 2}, result,
                               sequence{2, 4}) /
                 m_expr;
    }
  }

  // ddet(expr)/dX = ddet(expr)/dexpr:dexpr/dX
  //               = det(expr)*inv(trans(expr)):dexpr/dX
  void operator()([[maybe_unused]] tensor_det<ValueType> const &visitable) {
    auto result{diff(visitable.expr(), m_arg)};
    if (result.is_valid()) {
      m_result = m_expr * inner_product(inv(trans(visitable.expr())),
                                        sequence{1, 2}, result, sequence{2, 4});
    }
  }

  // d(-expr) = -dexpr
  void operator()(tensor_to_scalar_negative<ValueType> const &visitable) {
    m_result = -diff(visitable.expr(), m_arg);
  }

  void operator()(
      [[maybe_unused]] tensor_to_scalar_mul<ValueType> const &visitable) {}

  void operator()(
      [[maybe_unused]] tensor_to_scalar_add<ValueType> const &visitable) {}

  void operator()(
      [[maybe_unused]] tensor_to_scalar_sub<ValueType> const &visitable) {}

  void operator()(
      [[maybe_unused]] tensor_to_scalar_div<ValueType> const &visitable) {}

  void operator()(
      [[maybe_unused]] tensor_to_scalar_pow<ValueType> const &visitable) {}

  void operator()([[maybe_unused]] tensor_to_scalar_pow_with_scalar_exponent<
                  ValueType> const &visitable) {}

  void
  operator()([[maybe_unused]] tensor_to_scalar_with_scalar_mul<ValueType> const
                 &visitable) {}

  void
  operator()([[maybe_unused]] tensor_to_scalar_with_scalar_add<ValueType> const
                 &visitable) {}

  void
  operator()([[maybe_unused]] tensor_to_scalar_with_scalar_div<ValueType> const
                 &visitable) {}

  void
  operator()([[maybe_unused]] scalar_with_tensor_to_scalar_div<ValueType> const
                 &visitable) {}

  void
  operator()([[maybe_unused]] tensor_inner_product_to_scalar<ValueType> const
                 &visitable) {}

  template <typename Expr>
  void operator()([[maybe_unused]] Expr const &visitable) {}

private:
  result_expr_type const &m_arg;
  result_expr_type m_result{nullptr};
  expr_type m_expr;
  result_expr_type I;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_DIFFERENTIATION_H
