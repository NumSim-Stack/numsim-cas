#ifndef NUMSIM_CAS_PARSER_ERROR_MESSAGES_H
#define NUMSIM_CAS_PARSER_ERROR_MESSAGES_H

#include "grammar.h"

#include <tao/pegtl.hpp>
#include <tao/pegtl/must_if.hpp>

namespace numsim::cas::parser {

// User-facing text for the rules a must<>/if_must<> can raise on.
// pegtl::must_if static_asserts when such a rule has no message, so a new
// commit point in the grammar fails to compile until it gets one.
struct error_messages {
  template <typename Rule> static constexpr char const *message = nullptr;

  // Messages describe hard errors only: a rule that merely fails still
  // backtracks, so giving one a message must not turn it into an error.
  template <typename Rule> static constexpr bool raise_on_failure = false;
};

// Grammar root.
template <>
inline constexpr auto error_messages::message<grammar::expression> =
    "expected an expression";
template <>
inline constexpr auto error_messages::message<tao::pegtl::eof> =
    "unexpected trailing input";

// Function call: 'name(' commits, so the argument list and ')' must follow.
template <>
inline constexpr auto error_messages::message<grammar::function_call_close> =
    "expected ')' to close the function call";
template <>
inline constexpr auto
    error_messages::message<tao::pegtl::opt<grammar::arg_list>> =
        "expected an argument list";

// Bracket-list index literal: '[' commits.
template <>
inline constexpr auto error_messages::message<grammar::index_list_items> =
    "expected a 1-based index";
template <>
inline constexpr auto error_messages::message<grammar::index_list_close> =
    "expected ']' to close the index list";

// Tensor declaration: '{' commits.
template <>
inline constexpr auto error_messages::message<grammar::tensor_kv_list> =
    "expected 'rank=<n>' or 'dim=<n>'";
template <>
inline constexpr auto error_messages::message<grammar::tensor_decl_close> =
    "expected '}' to close the tensor declaration";

// Binary operators commit to a right operand, so the message names the
// operator the user just typed.
template <>
inline constexpr auto error_messages::message<grammar::add_rhs_plus> =
    "expected an expression after '+'";
template <>
inline constexpr auto error_messages::message<grammar::add_rhs_minus> =
    "expected an expression after '-'";
template <>
inline constexpr auto error_messages::message<grammar::mul_rhs_star> =
    "expected an expression after '*'";
template <>
inline constexpr auto error_messages::message<grammar::mul_rhs_slash> =
    "expected an expression after '/'";
template <>
inline constexpr auto error_messages::message<grammar::power_rhs> =
    "expected an expression after '^'";
template <>
inline constexpr auto error_messages::message<grammar::cmp_rhs_lt> =
    "expected an expression after '<'";
template <>
inline constexpr auto error_messages::message<grammar::cmp_rhs_le> =
    "expected an expression after '<='";
template <>
inline constexpr auto error_messages::message<grammar::cmp_rhs_gt> =
    "expected an expression after '>'";
template <>
inline constexpr auto error_messages::message<grammar::cmp_rhs_ge> =
    "expected an expression after '>='";
template <>
inline constexpr auto error_messages::message<grammar::eq_rhs_eq> =
    "expected an expression after '=='";
template <>
inline constexpr auto error_messages::message<grammar::eq_rhs_ne> =
    "expected an expression after '!='";

// Parenthesised subexpression: '(' commits.
template <>
inline constexpr auto error_messages::message<grammar::paren_close> =
    "expected ')' to close the parenthesis";

// A comma commits to another item in each of the three lists.
template <>
inline constexpr auto error_messages::message<grammar::arg_item_after_comma> =
    "expected an argument after ','";
template <>
inline constexpr auto error_messages::message<grammar::index_after_comma> =
    "expected a 1-based index after ','";
template <>
inline constexpr auto error_messages::message<grammar::tensor_kv_after_comma> =
    "expected 'rank=<n>' or 'dim=<n>' after ','";

// Padding inside the commit points. star<space> cannot fail, so this text
// is unreachable; must_if still requires it because must<> instantiates the
// raise path for every rule it guards.
template <>
inline constexpr auto error_messages::message<grammar::ws> =
    "expected whitespace";

template <typename Rule>
using error_control = tao::pegtl::must_if<error_messages>::control<Rule>;

} // namespace numsim::cas::parser

#endif // NUMSIM_CAS_PARSER_ERROR_MESSAGES_H
