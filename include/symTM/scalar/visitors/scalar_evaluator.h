#ifndef SCALAR_EVALUATOR_H
#define SCALAR_EVALUATOR_H

//#include "scalar.h"
//#include "scalar_add.h"
//#include "scalar_sub.h"
#include "../../symTM_type_traits.h"
#include <math.h>
#include <ranges>
#include <stdexcept>

namespace symTM {

/**
 * @brief A class to evaluate scalar expressions.
 *
 * The scalar_evaluator class provides functionality to evaluate mathematical
 * expressions involving scalars, such as addition, multiplication, negation,
 * and division. This evaluation is achieved through recursive application on
 * expression trees.
 *
 * @tparam ValueType The type of the values in the expressions.
 */
template <typename ValueType> class scalar_evaluator {
public:
  /**
   * @brief Default constructor for scalar_evaluator.
   */
  scalar_evaluator() = default;

  /// Delete the copy constructor.
  scalar_evaluator(scalar_evaluator const &) = delete;

  /// Delete the move constructor.
  scalar_evaluator(scalar_evaluator &&) = delete;

  /// Delete the copy assignment operator.
  scalar_evaluator &operator=(scalar_evaluator const &) = delete;

  /**
   * @brief Evaluates a given scalar expression.
   *
   * This method recursively evaluates the provided expression and returns the
   * computed result. The expression can be of various types, including scalar
   * constants, scalar addition, scalar multiplication, and others.
   *
   * @param expr The expression to evaluate.
   * @return ValueType The evaluated result of the expression.
   */
  ValueType apply(expression_holder<scalar_expression<ValueType>> &expr) {
    return std::visit([this](auto &arg) { return (*this)(arg); }, *expr);
  }

  /**
   * @brief Evaluates a given scalar expression.
   *
   * This method recursively evaluates the provided expression and returns the
   * computed result. The expression can be of various types, including scalar
   * constants, scalar addition, scalar multiplication, and others.
   *
   * @param expr The expression to evaluate.
   * @return ValueType The evaluated result of the expression.
   */
  ValueType apply(expression_holder<scalar_expression<ValueType>> &&expr) {
    return std::visit([this](auto &arg) { return (*this)(arg); }, *expr);
  }

  /**
   * @brief Evaluates a scalar variable or a sub-expression within a scalar.
   *
   * If the scalar contains an inner expression, it recursively evaluates the
   * expression. Otherwise, it returns the scalar’s data value.
   *
   * @param visitable The scalar variable.
   * @return ValueType The evaluated result of the scalar or its expression.
   */
  ValueType operator()(scalar<ValueType> &visitable) {
    return visitable.data();
  }

  /**
   * @brief Evaluates a scalar multiplication expression.
   *
   * Multiplies the evaluated results of each child expression within the scalar
   * multiplication expression.
   *
   * @param visitable The scalar multiplication expression to evaluate.
   * @return ValueType The product of all evaluated child expressions.
   */
  ValueType operator()(scalar_mul<ValueType> &visitable) {
    ValueType result{1};
    if (visitable.coeff().is_valid()) {
      result = apply(visitable.coeff());
    }
    for (auto &child : visitable.hash_map() | std::views::values) {
      result *= apply(child);
    }
    return result;
  }

  /**
   * @brief Evaluates a scalar addition expression.
   *
   * Adds the evaluated results of each child expression within the scalar
   * addition expression.
   *
   * @param visitable The scalar addition expression to evaluate.
   * @return ValueType The sum of all evaluated child expressions.
   */
  ValueType operator()(scalar_add<ValueType> &visitable) {
    ValueType result{0};
    if (visitable.coeff().is_valid()) {
      result += apply(visitable.coeff());
    }
    for (auto &child : visitable.hash_map() | std::views::values) {
      result += apply(child);
    }
    return result;
  }

  /**
   * @brief Evaluates a scalar subtraction expression.
   *
   * Subtracts the evaluated results of each child expression within the scalar
   * subtraction expression.
   *
   * @param visitable The scalar subtraction expression to evaluate.
   * @return ValueType The sum of all evaluated child expressions.
   */
  ValueType operator()(scalar_sub<ValueType> &visitable) {
    ValueType result{0};
    if (visitable.coeff().is_valid()) {
      result -= apply(visitable.coeff());
    }
    for (auto &child : visitable.hash_map() | std::views::values) {
      result -= apply(child);
    }
    return result;
  }

  /**
   * @brief Evaluates a scalar negation expression.
   *
   * Negates the result of the evaluated inner expression.
   *
   * @param visitable The scalar negation expression to evaluate.
   * @return ValueType The negated result of the inner expression.
   */
  ValueType operator()(scalar_negative<ValueType> &visitable) {
    return -apply(visitable.expr());
  }

  /**
   * @brief Evaluates a scalar division expression.
   *
   * Divides the evaluated left-hand expression by the right-hand expression.
   * Throws an exception if the right-hand expression evaluates to zero.
   *
   * @param visitable The scalar division expression to evaluate.
   * @return ValueType The result of the division.
   * @throws std::runtime_error If division by zero is encountered.
   */
  ValueType operator()(scalar_div<ValueType> &visitable) {
    const ValueType lhs{apply(visitable.expr_lhs())};
    const ValueType rhs{apply(visitable.expr_rhs())};
    if (rhs == 0) {
      throw std::runtime_error("Division by zero in scalar_div evaluation.");
    }
    return lhs / rhs;
  }

  /**
   * @brief Evaluates a scalar constant expression.
   *
   * Simply returns the constant value of the scalar.
   *
   * @param visitable The scalar constant expression to evaluate.
   * @return ValueType The constant value of the scalar.
   */
  ValueType operator()(scalar_constant<ValueType> &visitable) {
    return visitable();
  }

  ValueType operator()(scalar_one<ValueType> &visitable) {
    return static_cast<ValueType>(1);
  }

  ValueType operator()(scalar_zero<ValueType> &visitable) {
    return static_cast<ValueType>(0);
  }

  ValueType operator()(scalar_tan<ValueType> &visitable) {
    return std::tan(apply(visitable.expr()));
  }

  ValueType operator()(scalar_sin<ValueType> &visitable) {
    return std::sin(apply(visitable.expr()));
  }

  ValueType operator()(scalar_cos<ValueType> &visitable) {
    return std::cos(apply(visitable.expr()));
  }

  ValueType operator()(scalar_atan<ValueType> const &visitable) {
    return std::atan(apply(visitable.expr()));
  }

  ValueType operator()(scalar_asin<ValueType> const &visitable) {
    return std::asin(apply(visitable.expr()));
  }

  ValueType operator()(scalar_acos<ValueType> const &visitable) {
    return std::acos(apply(visitable.expr()));
  }

  ValueType operator()(scalar_sqrt<ValueType> const &visitable) {
    const ValueType temp{apply(visitable.expr())};
    if (temp < 0) {
      throw std::runtime_error("");
    }
    return std::sqrt(temp);
  }

  void operator()(scalar_log<ValueType> const &visitable) {
    return std::log(apply(visitable.expr()));
  }

  ValueType operator()(scalar_exp<ValueType> const &visitable) {
    return std::exp(apply(visitable.expr()));
  }

  ValueType operator()(scalar_pow<ValueType> const &visitable) {
    return std::pow(apply(visitable.expr_lhs()), apply(visitable.expr_rhs()));
  }

  ValueType operator()(scalar_sign<ValueType> const &visitable) {
    const ValueType temp{apply(visitable.expr())};
    return temp < 0 ? -1 : 1;
  }

  ValueType operator()(scalar_abs<ValueType> const &visitable) {
    return std::abs(apply(visitable.expr()));
  }
};

} // NAMESPACE symTM

#endif // SCALAR_EVALUATOR_H
