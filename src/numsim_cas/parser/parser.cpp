#include <numsim_cas/parser/parser.h>

#include "actions.h"
#include "grammar.h"

#include <numsim_cas/parser/parse_error.h>

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

// #355 — PEGTL parses by C++ recursion; deeply nested input overflows the
// stack (SIGSEGV at ~10-20k frames) instead of raising parse_error. A cheap
// pre-scan bounds every recursion driver: bracket nesting and unary-minus
// runs (whitespace does not reset a run: "- - -x" recurses per minus).
void check_nesting_depth(std::string_view source) {
  constexpr std::size_t max_depth = 512;
  const auto is_space = [](char c) {
    // must cover PEGTL's full space set or a whitespace variant resets
    // the run and bypasses the guard (review on #355)
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' ||
           c == '\f';
  };
  // Cumulative recursion budget along the current parse path (round-2
  // review): brackets, unary-minus runs, and ^-chains all contribute, and
  // runs at ENCLOSING bracket levels stay on the stack while inside -
  // independent counters let compound payloads (300 minuses per level x
  // 200 levels) or bracketed exponents (1^(1)^(1)...) multiply past any
  // per-feature cap. A ^-chain survives its bracketed exponent
  // (x^(y)^z is one chain), so the caret run is restored on close.
  std::size_t base = 0;      // cost contributed by enclosing levels
  std::size_t minus_run = 0; // current level
  std::size_t caret_run = 0; // current level
  std::vector<std::pair<std::size_t, std::size_t>> saved;
  const auto cost = [&]() { return base + minus_run + caret_run; };
  for (std::size_t i = 0; i < source.size(); ++i) {
    const char c = source[i];
    if (c == '(' || c == '[' || c == '{') {
      saved.emplace_back(minus_run, caret_run);
      base += minus_run + caret_run + 1;
      minus_run = 0;
      caret_run = 0;
      if (base > max_depth) {
        throw syntax_error("expression nesting too deep", i, source);
      }
    } else if (c == ')' || c == ']' || c == '}') {
      if (!saved.empty()) {
        auto [m, k] = saved.back();
        saved.pop_back();
        base -= m + k + 1;
        minus_run = 0; // the unary chain's operand just completed
        caret_run = k; // the ^-chain continues past its exponent
      }
    } else if (c == '^') {
      ++caret_run;
      if (cost() > max_depth) {
        throw syntax_error("expression nesting too deep", i, source);
      }
    } else if (c == '+' || c == '*' || c == '/' || c == ',') {
      minus_run = 0;
      caret_run = 0;
    } else if (c == '-') {
      caret_run = 0; // a binary/unary minus ends any ^-chain
      ++minus_run;
      if (cost() > max_depth) {
        throw syntax_error("expression nesting too deep", i, source);
      }
    } else if (!is_space(c)) {
      minus_run = 0; // operand characters end a unary-minus chain
    }
  }
}

} // namespace

parsed_expression parse(std::string_view source, symbol_table &syms) {
  check_nesting_depth(source);
  // Make a copy into a std::string-backed input — PEGTL's
  // memory_input takes ownership of the source view (it doesn't
  // copy) so callers must keep the string alive across the parse.
  // string_input copies the data internally, which is safer for our
  // public API where we accept a string_view by value.
  pegtl::string_input input{std::string(source), "<source>"};

  // #222 — roll back any declarations if the parse throws, so a failed
  // parse leaves the symbol_table unchanged (committed only on success).
  symbol_table::transaction tx(syms);

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
  auto result = std::visit(
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
  tx.commit();
  return result;
}

expression_holder<scalar_expression> parse_scalar(std::string_view source,
                                                  symbol_table &syms) {
  // #222/#314 — outer transaction so a domain mismatch rolls back the
  // declarations parse() committed internally (commit only on match).
  symbol_table::transaction tx(syms);
  auto result = parse(source, syms);
  if (auto *s = std::get_if<expression_holder<scalar_expression>>(&result)) {
    tx.commit();
    return std::move(*s);
  }
  throw type_mismatch_error(
      "parse_scalar called on expression that did not resolve to scalar", 0,
      source);
}

expression_holder<tensor_expression> parse_tensor(std::string_view source,
                                                  symbol_table &syms) {
  symbol_table::transaction tx(syms); // #222/#314 — roll back on mismatch
  auto result = parse(source, syms);
  if (auto *t = std::get_if<expression_holder<tensor_expression>>(&result)) {
    tx.commit();
    return std::move(*t);
  }
  throw type_mismatch_error(
      "parse_tensor called on expression that did not resolve to tensor", 0,
      source);
}

expression_holder<tensor_to_scalar_expression>
parse_t2s(std::string_view source, symbol_table &syms) {
  symbol_table::transaction tx(syms); // #222/#314 — roll back on mismatch
  auto result = parse(source, syms);
  if (auto *t = std::get_if<expression_holder<tensor_to_scalar_expression>>(
          &result)) {
    tx.commit();
    return std::move(*t);
  }
  throw type_mismatch_error(
      "parse_t2s called on expression that did not resolve to "
      "tensor_to_scalar",
      0, source);
}

} // namespace numsim::cas::parser
