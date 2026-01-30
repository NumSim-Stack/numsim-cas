// #ifndef SCALAR_EVALUATOR_H
// #define SCALAR_EVALUATOR_H

// #include "../../expression_holder.h" // if not already included by traits
// #include "../../numsim_cas_type_traits.h"
// #include <cmath>
// #include <ranges>
// #include <stdexcept>

// namespace numsim::cas {

// /**
//  * @brief Evaluates scalar CAS expressions to a numeric value.
//  *
//  * This visitor evaluates an expression \f$e\f$ to a value of type @p
//  ValueType
//  * by recursively evaluating child expressions and applying the corresponding
//  * mathematical operation.
//  *
//  * Supported operations include:
//  * \f[
//  *   +,\; -,\; \cdot,\; /,\; \sin,\; \cos,\; \tan,\; \arcsin,\; \arccos,\;
//  * \arctan,\; \sqrt{\cdot},\; \log(\cdot),\; \exp(\cdot),\;
//  * \operatorname{pow}(\cdot,\cdot),\;
//  *   |\cdot|,\; \operatorname{sgn}(\cdot).
//  * \f]
//  *
//  * @tparam ValueType The numeric type produced by evaluation (e.g. double).
//  */
// template <typename ValueType> class scalar_evaluator {
// public:
//   /// @brief Default constructor.
//   scalar_evaluator() = default;

//   scalar_evaluator(scalar_evaluator const &) = delete;
//   scalar_evaluator(scalar_evaluator &&) = delete;
//   scalar_evaluator &operator=(scalar_evaluator const &) = delete;

//   /**
//    * @brief Evaluate an expression.
//    *
//    * If @p expr is invalid, this implementation returns \f$0\f$.
//    *
//    * @param expr Expression to evaluate.
//    * @return Evaluated numeric value \f$e\f$.
//    */
//   ValueType
//   apply(expression_holder<scalar_expression> const &expr) const {
//     if (!expr.is_valid()) {
//       return static_cast<ValueType>(0);
//     }
//     return std::visit([this](auto const &arg) { return (*this)(arg); },
//     *expr);
//   }

//   /**
//    * @brief Evaluate an rvalue expression handle.
//    *
//    * @param expr Expression to evaluate.
//    * @return Evaluated numeric value.
//    */
//   ValueType
//   apply(expression_holder<scalar_expression> &&expr) const {
//     return apply(
//         static_cast<expression_holder<scalar_expression> const &>(
//             expr));
//   }

//   /**
//    * @brief Evaluate a scalar variable.
//    *
//    * Returns the stored value \f$x\f$.
//    *
//    * @param visitable Scalar variable node.
//    * @return \f$x\f$.
//    */
//   ValueType operator()(scalar const &visitable) const {
//     return visitable.data();
//   }

//   /**
//    * @brief Evaluate a scalar function node.
//    *
//    * Evaluates the function body expression \f$f(x)\f$.
//    *
//    * @param visitable Function node.
//    * @return \f$f(x)\f$ evaluated.
//    */
//   ValueType operator()(scalar_function const &visitable) const {
//     return apply(visitable.expr());
//   }

//   /**
//    * @brief Evaluate multiplication.
//    *
//    * For \f$e = c \cdot \prod_i a_i\f$, returns:
//    * \f[
//    *   c \cdot \prod_i a_i.
//    * \f]
//    *
//    * @param visitable Multiplication node.
//    * @return Product value.
//    */
//   ValueType operator()(scalar_mul const &visitable) const {
//     ValueType result{static_cast<ValueType>(1)};
//     if (visitable.coeff().is_valid()) {
//       result = apply(visitable.coeff());
//     }
//     for (auto const &child : visitable.hash_map() | std::views::values) {
//       result *= apply(child);
//     }
//     return result;
//   }

//   /**
//    * @brief Evaluate addition.
//    *
//    * For \f$e = c + \sum_i a_i\f$, returns:
//    * \f[
//    *   c + \sum_i a_i.
//    * \f]
//    *
//    * @param visitable Addition node.
//    * @return Sum value.
//    */
//   ValueType operator()(scalar_add const &visitable) const {
//     ValueType result{static_cast(0)};
//     if (visitable.coeff().is_valid()) {
//       result += apply(visitable.coeff());
//     }
//     for (auto const &child : visitable.hash_map() | std::views::values) {
//       result += apply(child);
//     }
//     return result;
//   }

//   /**
//    * @brief Evaluate negation.
//    *
//    * For \f$e = -u\f$, returns \f$-u\f$.
//    *
//    * @param visitable Negation node.
//    * @return \f$-u\f$.
//    */
//   ValueType operator()(scalar_negative const &visitable) const {
//     return -apply(visitable.expr());
//   }

//   /**
//    * @brief Evaluate division.
//    *
//    * For \f$e = \frac{g}{h}\f$, returns \f$g/h\f$.
//    *
//    * @throws std::runtime_error if \f$h=0\f$.
//    *
//    * @param visitable Division node.
//    * @return \f$g/h\f$.
//    */
//   ValueType operator()(scalar_div const &visitable) const {
//     const ValueType lhs{apply(visitable.expr_lhs())};
//     const ValueType rhs{apply(visitable.expr_rhs())};
//     if (rhs == static_cast(0)) {
//       throw std::runtime_error("Division by zero in scalar_div evaluation.");
//     }
//     return lhs / rhs;
//   }

//   /**
//    * @brief Evaluate a constant.
//    *
//    * @param visitable Constant node.
//    * @return \f$c\f$.
//    */
//   ValueType operator()(scalar_constant const &visitable) const {
//     return visitable();
//   }

//   /**
//    * @brief Evaluate one.
//    * @return \f$1\f$.
//    */
//   ValueType operator()(scalar_one const &) const {
//     return static_cast(1);
//   }

//   /**
//    * @brief Evaluate zero.
//    * @return \f$0\f$.
//    */
//   ValueType operator()(scalar_zero const &) const {
//     return static_cast<ValueType>(0);
//   }

//   /**
//    * @brief Evaluate \f$\tan(u)\f$.
//    * @return \f$\tan(u)\f$.
//    */
//   ValueType operator()(scalar_tan const &visitable) const {
//     return std::tan(apply(visitable.expr()));
//   }

//   /**
//    * @brief Evaluate \f$\sin(u)\f$.
//    * @return \f$\sin(u)\f$.
//    */
//   ValueType operator()(scalar_sin const &visitable) const {
//     return std::sin(apply(visitable.expr()));
//   }

//   /**
//    * @brief Evaluate \f$\cos(u)\f$.
//    * @return \f$\cos(u)\f$.
//    */
//   ValueType operator()(scalar_cos const &visitable) const {
//     return std::cos(apply(visitable.expr()));
//   }

//   /**
//    * @brief Evaluate \f$\arctan(u)\f$.
//    * @return \f$\arctan(u)\f$.
//    */
//   ValueType operator()(scalar_atan const &visitable) const {
//     return std::atan(apply(visitable.expr()));
//   }

//   /**
//    * @brief Evaluate \f$\arcsin(u)\f$.
//    * @return \f$\arcsin(u)\f$.
//    */
//   ValueType operator()(scalar_asin const &visitable) const {
//     return std::asin(apply(visitable.expr()));
//   }

//   /**
//    * @brief Evaluate \f$\arccos(u)\f$.
//    * @return \f$\arccos(u)\f$.
//    */
//   ValueType operator()(scalar_acos const &visitable) const {
//     return std::acos(apply(visitable.expr()));
//   }

//   /**
//    * @brief Evaluate \f$\sqrt{u}\f$.
//    *
//    * @throws std::runtime_error if \f$u < 0\f$ (real-valued evaluation).
//    *
//    * @param visitable Square-root node.
//    * @return \f$\sqrt{u}\f$.
//    */
//   ValueType operator()(scalar_sqrt const &visitable) const {
//     const ValueType u{apply(visitable.expr())};
//     if (u < static_cast(0)) {
//       throw std::runtime_error(
//           "sqrt: negative argument in real-valued evaluation.");
//     }
//     return std::sqrt(u);
//   }

//   /**
//    * @brief Evaluate natural logarithm \f$\log(u)\f$.
//    *
//    * Interprets \f$\log\f$ as the natural logarithm.
//    *
//    * @throws std::runtime_error if \f$u \le 0\f$ (real-valued evaluation).
//    *
//    * @param visitable Log node.
//    * @return \f$\log(u)\f$.
//    */
//   ValueType operator()(scalar_log const &visitable) const {
//     const ValueType u{apply(visitable.expr())};
//     if (u <= static_cast(0)) {
//       throw std::runtime_error(
//           "log: non-positive argument in real-valued evaluation.");
//     }
//     return std::log(u);
//   }

//   /**
//    * @brief Evaluate \f$\exp(u)\f$.
//    * @return \f$e^{u}\f$.
//    */
//   ValueType operator()(scalar_exp const &visitable) const {
//     return std::exp(apply(visitable.expr()));
//   }

//   /**
//    * @brief Evaluate power \f$\operatorname{pow}(g,h)\f$.
//    * @return \f$g^{h}\f$.
//    */
//   ValueType operator()(scalar_pow const &visitable) const {
//     return std::pow(apply(visitable.expr_lhs()),
//     apply(visitable.expr_rhs()));
//   }

//   /**
//    * @brief Evaluate sign function \f$\operatorname{sgn}(u)\f$.
//    *
//    * \f[
//    * \operatorname{sgn}(u)=
//    * \begin{cases}
//    * -1 & u<0
//    * 0  & u=0
//    * 1  & u>0
//    * \end{cases}
//    * \f]
//    *
//    * @return \f$\operatorname{sgn}(u)\f$.
//    */
//   ValueType operator()(scalar_sign const &visitable) const {
//     const ValueType u{apply(visitable.expr())};
//     if (u > static_cast<ValueType>(0))
//       return static_cast<ValueType>(1);
//     if (u < static_cast<ValueType>(0))
//       return static_cast<ValueType>(-1);
//     return static_cast<ValueType>(0);
//   }

//   /**
//    * @brief Evaluate absolute value \f$|u|\f$.
//    * @return \f$|u|\f$.
//    */
//   ValueType operator()(scalar_abs const &visitable) const {
//     return std::abs(apply(visitable.expr()));
//   }

//   /**
//    * @brief Catch-all to make missing overloads a compile-time error.
//    *
//    * If a new node type is added to the variant and no overload exists here,
//    * compilation fails with a clear message.
//    */
//   template <class T> ValueType operator()(T const &) const {
//     static_assert(sizeof(T) == 0,
//                   "scalar_evaluator: missing overload for this node type");
//     return static_cast<ValueType>(0);
//   }
// };

// } // namespace numsim::cas

// #endif // SCALAR_EVALUATOR_H
