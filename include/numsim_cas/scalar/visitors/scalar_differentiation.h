#ifndef SCALAR_DIFFERENTIATION_H
#define SCALAR_DIFFERENTIATION_H

#include "../../basic_functions.h"
#include "../../expression_holder.h"
#include "../../numsim_cas_type_traits.h"
#include "../../operators.h"
#include <ranges>

#include "../scalar_add.h"
#include "../scalar_globals.h"
#include "../scalar_mul.h"
#include "../scalar_one.h"
#include "../scalar_std.h"
#include "../scalar_sub.h"
#include "../scalar_zero.h"

namespace numsim::cas {

template <typename ValueType> class scalar_differentiation {
public:
  using expr_t = expression_holder<scalar_expression<ValueType>>;

  scalar_differentiation(expr_t const &arg) : m_arg(arg) {}
  scalar_differentiation(scalar_differentiation const &) = delete;
  scalar_differentiation(scalar_differentiation &&) = delete;
  const scalar_differentiation &
  operator=(scalar_differentiation const &) = delete;

  auto apply(expr_t &expr) { return apply_imp(expr); }

  auto apply(expr_t const &expr) { return apply_imp(expr); }

  auto apply(expr_t &&expr) { return apply_imp(expr); }

  void operator()(scalar<ValueType> const &visitable) {
    if (&visitable == &m_arg.get()) {
      m_result = get_scalar_one<ValueType>();
    } else {
      m_result = get_scalar_zero<ValueType>();
    }
  }

  void operator()(scalar_function<ValueType> const &visitable) {
    scalar_differentiation<ValueType> diff(m_arg);
    auto result{diff.apply(visitable.expr())};
    if (result.is_valid()) {
      m_result = make_expression<scalar_function<ValueType>>(
          "d" + visitable.name(), result);
    }
  }

  /// product rule
  /// f(x)  = c * prod_i^n a_i(x)
  /// f'(x)  = c * sum_j^n a_j(x) prod_i^{n, i\neq j} a_i(x)
  /// TODO: just copy the vector and manipulate the current entry
  void operator()(scalar_mul<ValueType> const &visitable) {
    expr_t expr_result;
    for (auto &expr_out : visitable.hash_map() | std::views::values) {
      expr_t expr_result_in;
      for (auto &expr_in : visitable.hash_map() | std::views::values) {
        if (expr_out == expr_in) {
          scalar_differentiation<ValueType> diff(m_arg);
          expr_result_in *= diff.apply(expr_in);
        } else {
          expr_result_in *= expr_in;
        }
      }
      expr_result += expr_result_in;
    }
    if (visitable.coeff().is_valid()) {
      m_result = std::move(expr_result) * visitable.coeff();
    } else {
      m_result = std::move(expr_result);
    }
  }

  /// summation rule
  /// f(x)  = c + sum_i^n a_i(x)
  /// f'(x) = sum_i^n a_i'(x)
  void operator()([[maybe_unused]] scalar_add<ValueType> const &visitable) {
    expr_t expr_result;
    auto add{make_expression<scalar_add<ValueType>>()};
    for (auto &child : visitable.hash_map() | std::views::values) {
      scalar_differentiation diff(m_arg);
      expr_result += diff.apply(child);
    }
    m_result = std::move(expr_result);
  }

  void operator()(scalar_negative<ValueType> const &visitable) {
    scalar_differentiation diff(m_arg);
    auto diff_expr{diff.apply(visitable.expr())};
    if (diff_expr.is_valid() || !is_same<scalar_zero<ValueType>>(diff_expr)) {
      m_result = -diff_expr;
    }
  }

  /// Quotientenregel
  /// f(x) = g(x)/h(x)
  /// f(x) = (g(x)'*h(x) - g(x)*h(x)')/(h(x)*h(x)))
  /// g(x) := 0
  /// f(x) = (h(x) - g(x)*h(x)')/(h(x)*h(x)))
  /// h(x) := 0
  /// f(x) = (g(x)'*h(x) - g(x))/(h(x)*h(x)))
  void operator()(scalar_div<ValueType> const &visitable) {
    auto g{visitable.expr_lhs()};
    auto h{visitable.expr_rhs()};
    scalar_differentiation<ValueType> diff(m_arg);
    auto dg{diff.apply(visitable.expr_lhs())};
    auto dh{diff.apply(visitable.expr_rhs())};
    m_result = (dg * h - g * dh) / (h * h);
  }

  void
  operator()([[maybe_unused]] scalar_constant<ValueType> const &visitable) {
    m_result = get_scalar_zero<ValueType>();
  }

  // tan(x)' = sec^2(x) = (1/cos(x))^2
  void operator()([[maybe_unused]] scalar_tan<ValueType> const &visitable) {
    m_result = std::pow(1 / std::cos(visitable.expr()), 2);
  }

  void operator()([[maybe_unused]] scalar_sin<ValueType> const &visitable) {
    m_result = std::cos(visitable.expr());
    apply_inner_unary(visitable);
  }

  void operator()([[maybe_unused]] scalar_cos<ValueType> const &visitable) {
    m_result = -std::sin(visitable.expr());
    apply_inner_unary(visitable);
  }

  void operator()([[maybe_unused]] scalar_one<ValueType> const &visitable) {
    m_result = get_scalar_zero<ValueType>();
  }

  void operator()([[maybe_unused]] scalar_zero<ValueType> const &visitable) {
    m_result = get_scalar_zero<ValueType>();
  }

  // 1/(expr^2+1)
  void operator()([[maybe_unused]] scalar_atan<ValueType> const &visitable) {
    auto &one{get_scalar_one<ValueType>()};
    m_result = (one / (one + std::pow(visitable.expr(), 2)));
    apply_inner_unary(visitable);
  }

  // 1/sqrt(1-expr^2)
  void operator()([[maybe_unused]] scalar_asin<ValueType> const &visitable) {
    auto &one{get_scalar_one<ValueType>()};
    m_result = (one / (std::sqrt(one - std::pow(visitable.expr(), 2))));
    apply_inner_unary(visitable);
  }

  //-1/sqrt(1-expr^2)
  void operator()([[maybe_unused]] scalar_acos<ValueType> const &visitable) {
    auto &one{get_scalar_one<ValueType>()};
    m_result = -(one / (std::sqrt(one - std::pow(visitable.expr(), 2))));
    apply_inner_unary(visitable);
  }

  void operator()([[maybe_unused]] scalar_sqrt<ValueType> const &visitable) {
    auto &one{get_scalar_one<ValueType>()};
    m_result = one / (2 * m_expr);
    apply_inner_unary(visitable);
  }

  void operator()([[maybe_unused]] scalar_exp<ValueType> const &visitable) {
    m_result = m_expr;
    apply_inner_unary(visitable);
  }

  void operator()([[maybe_unused]] scalar_pow<ValueType> const &visitable) {
    auto &expr_lhs{visitable.expr_lhs()};
    auto &expr_rhs{visitable.expr_rhs()};
    scalar_differentiation<ValueType> diff(m_arg);
    if (is_same<scalar_constant<ValueType>>(expr_rhs)) {
      // f(x)  = pow(g(x),c)
      // f'(x) = c*pow(g(x), c-1)*g'(x)
      const auto c_value{expr_rhs.template get<scalar_constant<ValueType>>()() -
                         static_cast<ValueType>(1)};
      if (c_value == 1) {
        m_result = expr_rhs * expr_lhs;
      } else {
        m_result =
            expr_rhs *
            std::pow(expr_lhs,
                     make_expression<scalar_constant<ValueType>>(c_value));
      }
    } else {
      // Implizite
      // f(x) = pow(g(x),h(x))
      // f'(x) = f(x)*(h'(x)*log(g(x)) + h(x)*g'(x)/g(x))
      // f'(x) = g(x)^h(x)*(h'(x)*log(g(x)) + h(x)*g'(x)/g(x)) |*g(x)
      // f'(x) = pow(g(x),(h(x)-1))*(h'(x)*log(g(x))*g(x) + h(x)*g'(x))
      auto &g{expr_lhs};
      auto &h{expr_rhs};
      scalar_differentiation<ValueType> diff(m_arg);
      auto dg{diff.apply(g)};
      auto dh{diff.apply(h)};
      if (is_same<scalar_zero<ValueType>>(dh)) {
        m_result = h * std::pow(g, h - 1) * dg;
      } else {
        m_result = std::pow(g, h - 1) * dh * std::log(std::pow(g, 2) + h * dg);
      }
    }
  }

  void operator()([[maybe_unused]] scalar_sign<ValueType> const &visitable) {
    m_result = get_scalar_zero<ValueType>();
  }

  void operator()([[maybe_unused]] scalar_abs<ValueType> const &visitable) {
    m_result = visitable.expr() / m_expr;
    apply_inner_unary(visitable);
  }

  void operator()([[maybe_unused]] scalar_log<ValueType> const &visitable) {
    auto &one{get_scalar_one<ValueType>()};
    m_result = one / visitable.expr();
    apply_inner_unary(visitable);
  }

private:
  template <typename T> void apply_inner_unary(T const &unary) {
    scalar_differentiation<ValueType> diff(m_arg);
    auto inner{diff.apply(unary.expr())};
    if (inner.is_valid()) {
      m_result *= std::move(inner);
    }
  }
  auto apply_imp(expr_t const &expr) {
    if (expr.is_valid()) {
      m_expr = expr;
      std::visit([this](auto &&arg) { (*this)(arg); }, *expr);
      return m_result;
    } else {
      return get_scalar_zero<ValueType>();
    }
  }

  expr_t const &m_arg;
  expr_t m_expr;
  expr_t m_result;
};

} // namespace numsim::cas
#endif // SCALAR_DIFFERENTIATION_H
