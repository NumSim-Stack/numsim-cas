#include <numsim_cas/parser/parser.h>

#include "actions.h"
#include "grammar.h"

#include <numsim_cas/parser/parse_error.h>

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace numsim::cas::parser {

namespace pegtl = tao::pegtl;

namespace {

// Translate PEGTL's own parse_error (raised when `must<>` rules fail
// or when a default action wraps an exception) into our syntax_error,
// preserving position info.
//
// Return type is `syntax_error` (NOT the base `parse_error`) — a
// return-by-value of the base type would slice the subclass off, and
// the subsequent `throw` would propagate a sliced base-only object.
// Callers downstream that catch by `syntax_error` would then miss it.
syntax_error translate_pegtl_error(pegtl::parse_error const &e,
                                   std::string_view source) {
  // pegtl::parse_error stores positions; pull the first.
  std::size_t byte = 0;
  if (!e.positions().empty()) {
    byte = e.positions().front().byte;
  }
  // Use e.what() (std::exception interface) — pegtl::parse_error
  // returns a string_view from message() which can't directly
  // construct a std::string by '='.
  std::string msg(e.message());
  return syntax_error(std::move(msg), byte, source);
}

} // namespace

parsed_expression parse(std::string_view source, symbol_table &syms) {
  // Make a copy into a std::string-backed input — PEGTL's
  // memory_input takes ownership of the source view (it doesn't
  // copy) so callers must keep the string alive across the parse.
  // string_input copies the data internally, which is safer for our
  // public API where we accept a string_view by value.
  pegtl::string_input input{std::string(source), "<source>"};

  actions::parser_state state(syms, source);

  try {
    pegtl::parse<grammar::grammar, actions::action>(input, state);
  } catch (parse_error const &) {
    // One of our actions threw — already has the right type +
    // position + snippet. Propagate.
    throw;
  } catch (pegtl::parse_error const &e) {
    // PEGTL's internal error — translate to our type.
    throw translate_pegtl_error(e, source);
  }

  if (state.values.size() != 1) {
    throw syntax_error("parser ended with " +
                           std::to_string(state.values.size()) +
                           " expressions on the stack (expected exactly 1)",
                       source.size(), source);
  }
  // Convert from the parser-internal `parser_value` (which can hold an
  // index_list_value as a 4th alternative) to the public
  // `parsed_expression`. An index_list_value at the top is a syntax
  // error — bracket-list literals are only valid inside contraction
  // function arg lists, never as a top-level expression.
  return std::visit(
      [&](auto &&v) -> parsed_expression {
        using V = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<V, registry::index_list_value>) {
          throw syntax_error(
              "bracket-list literal '[...]' cannot be a top-level expression",
              source.size(), source);
        } else {
          return std::move(v);
        }
      },
      std::move(state.values.front()));
}

expression_holder<scalar_expression> parse_scalar(std::string_view source,
                                                  symbol_table &syms) {
  auto result = parse(source, syms);
  if (auto *s = std::get_if<expression_holder<scalar_expression>>(&result)) {
    return std::move(*s);
  }
  throw type_mismatch_error(
      "parse_scalar called on expression that did not resolve to scalar", 0,
      source);
}

expression_holder<tensor_expression> parse_tensor(std::string_view source,
                                                  symbol_table &syms) {
  auto result = parse(source, syms);
  if (auto *t = std::get_if<expression_holder<tensor_expression>>(&result)) {
    return std::move(*t);
  }
  throw type_mismatch_error(
      "parse_tensor called on expression that did not resolve to tensor", 0,
      source);
}

expression_holder<tensor_to_scalar_expression>
parse_t2s(std::string_view source, symbol_table &syms) {
  auto result = parse(source, syms);
  if (auto *t = std::get_if<expression_holder<tensor_to_scalar_expression>>(
          &result)) {
    return std::move(*t);
  }
  throw type_mismatch_error(
      "parse_t2s called on expression that did not resolve to "
      "tensor_to_scalar",
      0, source);
}

} // namespace numsim::cas::parser
