#ifndef SCALAR_DIFFERENTIATION_H
#define SCALAR_DIFFERENTIATION_H

#include <ranges>
#include "../../operators.h"
#include "../../basic_functions.h"
#include "../../expression_holder.h"
#include "../../numsim_cas_type_traits.h"

#include "../scalar_globals.h"
#include "../scalar_one.h"
#include "../scalar_zero.h"
#include "../scalar_sub.h"
#include "../scalar_mul.h"
#include "../scalar_add.h"
#include "../scalar_std.h"

namespace numsim::cas {

template <typename ValueType>
class scalar_differentiation
{
public:
  using argument_type = expression_holder<scalar_expression<ValueType>>;

  scalar_differentiation(argument_type const &arg) : m_arg(arg) {}
  scalar_differentiation(scalar_differentiation const &) = delete;
  scalar_differentiation(scalar_differentiation &&) = delete;
  const scalar_differentiation &
  operator=(scalar_differentiation const &) = delete;

  auto apply(expression_holder<scalar_expression<ValueType>> &expr) {
    if(expr.is_valid()){
      std::visit([this](auto &&arg) { (*this)(arg); }, *expr);
      return m_result;
    }else{
      return get_scalar_zero<ValueType>();
    }
  }

  auto apply(expression_holder<scalar_expression<ValueType>> &&expr) {
    if(expr.is_valid()){
      std::visit([this](auto &&arg) { (*this)(arg); }, *expr);
      return m_result;
    }else{
      return get_scalar_zero<ValueType>();
    }
  }

  void operator()(scalar<ValueType> &visitable){
    if (&visitable == &m_arg.get()) {
      m_result = get_scalar_one<ValueType>();
    } else {
      m_result = get_scalar_zero<ValueType>();
    }
  }

  /// product rule
  /// f(x)  = c * prod_i^n a_i(x)
  /// f'(x)  = c * sum_j^n a_j(x) prod_i^{n, i\neq j} a_i(x)
  /// TODO: just copy the vector and manipulate the current entry
  void operator()(scalar_mul<ValueType> &visitable){
      expression_holder<scalar_expression<ValueType>> expr_result;
      for(auto &expr_out : visitable.hash_map() | std::views::values){
        expression_holder<scalar_expression<ValueType>> expr_result_in;
        for(auto &expr_in : visitable.hash_map() | std::views::values){
          if(expr_out == expr_in){
            scalar_differentiation<ValueType> diff(m_arg);
            expr_result_in *= diff.apply(expr_in);
          }else{
            expr_result_in *= expr_in;
          }
        }
        expr_result += expr_result_in;
      }
      if(visitable.coeff().is_valid()){
        m_result = std::move(expr_result) * visitable.coeff();
      }else{
        m_result = std::move(expr_result);
      }
  }

  /// summation rule
  /// f(x)  = c + sum_i^n a_i(x)
  /// f'(x) = sum_i^n a_i'(x)
  void operator()([[maybe_unused]]scalar_add<ValueType> &visitable){
    expression_holder<scalar_expression<ValueType>> expr_result;
    auto add{make_expression<scalar_add<ValueType>>()};
    for (auto &child : visitable.hash_map() | std::views::values) {
      scalar_differentiation diff(m_arg);
      expr_result += diff.apply(child);
    }
    m_result = std::move(expr_result);
  }

    //void operator()([[maybe_unused]]scalar_sub<ValueType> &visitable){}

  void operator()(scalar_negative<ValueType> &visitable){
    scalar_differentiation diff(m_arg);
    auto diff_expr{diff.apply(visitable.expr())};
    if (diff_expr.is_valid()) {
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
  void operator()(scalar_div<ValueType> &visitable){
    auto g{visitable.expr_lhs()};
    auto h{visitable.expr_rhs()};
    scalar_differentiation<ValueType> diff(m_arg);
    auto dg{diff.apply(visitable.expr_lhs())};
    auto dh{diff.apply(visitable.expr_rhs())};
    m_result = (dg*h - g*dh)/(h*h);
  }

  void operator()([[maybe_unused]]scalar_constant<ValueType> &visitable){
    m_result = get_scalar_zero<ValueType>();
  }

  void operator()([[maybe_unused]]scalar_tan<ValueType>&visitable){
  }

  void operator()([[maybe_unused]]scalar_sin<ValueType>&visitable){
  }

  void operator()([[maybe_unused]]scalar_cos<ValueType>&visitable){
  }

  void operator()([[maybe_unused]]scalar_one<ValueType>&visitable){
    m_result = get_scalar_zero<ValueType>();
  }

  void operator()([[maybe_unused]]scalar_zero<ValueType>&visitable){
    m_result = get_scalar_zero<ValueType>();
  }

  void operator()([[maybe_unused]]scalar_atan<ValueType>&visitable){
  }

  void operator()([[maybe_unused]]scalar_asin<ValueType>&visitable){
  }

  void operator()([[maybe_unused]]scalar_acos<ValueType>&visitable){
  }

  void operator()([[maybe_unused]]scalar_sqrt<ValueType>&visitable){
  }

  void operator()([[maybe_unused]]scalar_exp<ValueType>&visitable){
  }


  void operator()([[maybe_unused]]scalar_pow<ValueType>&visitable){
    auto& expr_lhs{visitable.expr_lhs()};
    auto& expr_rhs{visitable.expr_rhs()};
    scalar_differentiation<ValueType> diff(m_arg);
    if(is_same<scalar_constant<ValueType>>(expr_rhs)){
      //f(x)  = pow(g(x),c)
      //f'(x) = c*pow(g(x), c-1)
      const auto c_value{expr_rhs.template get<scalar_constant<ValueType>>()()-static_cast<ValueType>(1)};
      if(c_value == 1){
        m_result = expr_rhs*expr_lhs;
      }else{
        m_result = expr_rhs*std::pow(expr_lhs, make_expression<scalar_constant<ValueType>>(c_value));
      }
    }else{
      auto temp{diff.apply(expr_rhs)};
      assert(false);
    }
  }

  void operator()([[maybe_unused]]scalar_sign<ValueType>&visitable){
  }

  void operator()([[maybe_unused]]scalar_abs<ValueType>&visitable){
  }

  void operator()([[maybe_unused]]scalar_log<ValueType>&visitable){
  }

private:
  argument_type const &m_arg;
  expression_holder<scalar_expression<ValueType>> m_result;
};

} // namespace numsim::cas
#endif // SCALAR_DIFFERENTIATION_H
