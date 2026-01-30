#ifndef SCALAR_FUNCTIONS_H
#define SCALAR_FUNCTIONS_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/numsim_cas_type_traits.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

bool is_scalar_constant(
    expression_holder<scalar_expression> const &expr) noexcept;

std::optional<scalar_number>
get_scalar_number(expression_holder<scalar_expression> const &expr);

// Returns integer value if the expression is exactly an integer constant.
// Supports: scalar_zero, scalar_one, scalar_constant(k),
// scalar_negative(constant/one/zero).
std::optional<long long>
try_int_constant(expression_holder<scalar_expression> const &e);

bool is_integer_exponent(expression_holder<scalar_expression> const &exp);

// Combines:
//  1) pow(x,a) * pow(x,b) -> pow(x,a+b)
//  2) pow(x,a) * pow(y,a) -> pow(x*y,a)   (only if a is integer)
std::optional<expression_holder<scalar_expression>>
simplify_scalar_pow_pow_mul(scalar_pow const &lhs, scalar_pow const &rhs);

bool is_constant(expression_holder<scalar_expression> const &expr);

// template <std::integral I>
// inline std::optional<I>
// integer_constant_canonical(expression_holder<scalar_expression> const& expr)
// noexcept {
//   if (is_same<scalar_zero>(expr)) return I{0};
//   if (is_same<scalar_one>(expr))  return I{1};
//   return integer_constant<I>(expr); // strict scalar_constant integer
// }

// namespace detail {

// template <typename ValueType> class contains_symbol {
// public:
//   contains_symbol() {}

//   template <typename T> constexpr inline bool operator()(T const &) {
//     return false;
//   }

//   constexpr inline bool
//   operator()([[maybe_unused]] scalar<ValueType> const &visitable) {
//     return true;
//   }

//   template <typename Derived>
//   constexpr inline bool
//   operator()(unary_op<Derived, scalar_expression<ValueType>> const
//   &visitable) {
//     return std::visit(*this, visitable.expr().get());
//   }

//   //  template <typename Derived>
//   //  constexpr inline bool operator()(
//   //      binary_op<Derived, scalar_expression<ValueType>> const &visitable)
//   {
//   //    return std::visit(*this, visitable.expr_lhs().get()) ||
//   //           std::visit(*this, visitable.expr_rhs().get());
//   //  }

//   template <typename Derived>
//   constexpr inline bool operator()(
//       n_ary_tree<scalar_expression<ValueType>, Derived> const &visitable) {
//     for (auto &child : visitable.hash_map() | std::views::values) {
//       if (std::visit(*this, child.get())) {
//         return true;
//       }
//     }
//     return false;
//   }
// };
// } // namespace detail

// template <typename ValueType>
// bool contains_symbol(
//     expression_holder<scalar_expression<ValueType>> const &expr) {
//   return std::visit(detail::contains_symbol<ValueType>(), *expr);
// }

// template <typename StreamType>
// void
// print(StreamType &out,
//       expression_holder<scalar_expression> const &expr,
//       Precedence precedence = Precedence::None) {
//   scalar_printer<StreamType> eval(out);
//   eval.apply(expr, precedence);
// }

} // namespace numsim::cas
#endif // SCALAR_FUNCTIONS_H
