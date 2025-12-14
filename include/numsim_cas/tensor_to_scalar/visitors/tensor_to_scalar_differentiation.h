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
    if (!m_result.is_valid()) {
      return make_expression<tensor_zero<value_type>>(m_arg.get().dim(),
                                                      m_arg.get().rank());
    }
    return m_result;
  }

  // trace(expr) = I:expr/dim(expr) = expr_ii/dim(expr)
  // dtrace(expr)/dX = dtrace(expr)/dexpr:dexpr/dX
  //                 = I:dexpr
  void operator()([[maybe_unused]] tensor_trace<ValueType> const &visitable) {
    auto result{diff(visitable.expr(), m_arg)};
    m_result = inner_product(I, sequence{1, 2}, result, sequence{1, 2});
  }

  // d(expr_ij expr_ij)/dx_k = dexpr_ij/dx_k*expr_ij + expr_ij*dexpr_ij/dx_k
  //                         = 2*expr [*] dexpr/dx
  void operator()(tensor_dot<ValueType> const &visitable) {
    auto result{diff(visitable.expr(), m_arg)};
    if (result.is_valid()) {
      constexpr auto two{static_cast<value_type>(2)};
      if (visitable.expr().get().rank() == 1) {
        m_result = two * visitable.expr() * std::move(result);
      }
      sequence indices(visitable.expr().get().rank());
      std::iota(indices.begin(), indices.end(), 1);
      m_result = two * inner_product(visitable.expr(), indices,
                                     std::move(result), indices);
    }
  }

  // dnorm(expr)/dX = dsqrt(dot(expr,expr))/dX
  //                = dnorm(expr)/dexpr : dexpr/dX
  //                = expr/norm(expr) : dexpr/dX
  void operator()([[maybe_unused]] tensor_norm<ValueType> const &visitable) {
    auto result{diff(visitable.expr(), m_arg)};
    if (result.is_valid()) {
      m_result = inner_product(visitable.expr(), sequence{1, 2}, result,
                               sequence{1, 2}) /
                 m_expr;
    }
  }

  // ddet(expr)/dX = ddet(expr)/dexpr:dexpr/dX
  //               = det(expr)*inv(trans(expr)):dexpr/dX
  void operator()([[maybe_unused]] tensor_det<ValueType> const &visitable) {
    auto result{diff(visitable.expr(), m_arg)};
    if (result.is_valid()) {
      m_result = m_expr * inner_product(inv(trans(visitable.expr())),
                                        sequence{1, 2}, result, sequence{1, 2});
    }
  }

  // d(-expr) = -dexpr
  void operator()(tensor_to_scalar_negative<ValueType> const &visitable) {
    m_result = -diff(visitable.expr(), m_arg);
  }

  /// product rule
  /// f(x)  = c * prod_i^n a_i(x)
  /// f'(x)  = c * sum_j^n a_j(x) prod_i^{n, i\neq j} a_i(x)
  void operator()(
      [[maybe_unused]] tensor_to_scalar_mul<ValueType> const &visitable) {
    result_expr_type expr_result;
    for (auto &expr_out : visitable.hash_map() | std::views::values) {
      result_expr_type expr_result_in;
      // first get the diff
      expr_result_in *= diff(expr_out, m_arg);

      // now check if the diff is valid and not zero
      if (expr_result_in.is_valid() &&
          is_same<tensor_zero<value_type>>(expr_result_in)) {
        for (auto &expr_in : visitable.hash_map() | std::views::values) {
          expr_result_in = std::move(std::move(expr_result_in) * expr_in);
        }
      }
      expr_result += expr_result_in;
    }
    m_result = std::move(expr_result);
  }

  /// summation rule
  /// f(x)  = c + sum_i^n a_i(x)
  /// f'(x) = sum_i^n a_i'(x)
  void operator()(
      [[maybe_unused]] tensor_to_scalar_add<ValueType> const &visitable) {
    result_expr_type expr_result;
    for (auto &child : visitable.hash_map() | std::views::values) {
      expr_result += diff(child, m_arg);
    }
    m_result = std::move(expr_result);
  }

  void operator()(
      [[maybe_unused]] tensor_to_scalar_sub<ValueType> const &visitable) {}

  /// f(x) = g(x)/h(x)
  /// f(x) = (g(x)'*h(x) - g(x)*h(x)')/(h(x)*h(x)))
  /// g(x) := 0
  /// f(x) = (h(x) - g(x)*h(x)')/(h(x)*h(x)))
  /// h(x) := 0
  /// f(x) = (g(x)'*h(x) - g(x))/(h(x)*h(x)))
  void operator()(tensor_to_scalar_div<ValueType> const &visitable) {
    auto g{visitable.expr_lhs()};
    auto h{visitable.expr_rhs()};
    auto dg{diff(visitable.expr_lhs(), m_arg)};
    auto dh{diff(visitable.expr_rhs(), m_arg)};
    m_result = (dg * h - g * dh) / (h * h);
  }

  void operator()(
      [[maybe_unused]] tensor_to_scalar_pow<ValueType> const &visitable) {}

  void operator()([[maybe_unused]] tensor_to_scalar_pow_with_scalar_exponent<
                  ValueType> const &visitable) {}

  void
  operator()(tensor_to_scalar_with_scalar_mul<ValueType> const &visitable) {
    m_result = visitable.expr_lhs() * diff(visitable.expr_rhs(), m_arg);
  }

  void
  operator()(tensor_to_scalar_with_scalar_add<ValueType> const &visitable) {
    m_result = diff(visitable.expr_rhs(), m_arg);
  }

  void
  operator()(tensor_to_scalar_with_scalar_div<ValueType> const &visitable) {
    m_result = diff(visitable.expr_lhs(), m_arg) / visitable.expr_rhs();
  }

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
