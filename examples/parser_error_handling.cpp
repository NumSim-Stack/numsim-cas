// Parser error handling — demonstrate the diagnostic story.
//
// Every parse failure raises a specific parse_error subclass. The
// formatted what() includes a "parse error at line:col:" header, the
// offending source line, and a caret. Subclasses carry extra fields
// for structured handling (e.g. unknown_function_error::name(),
// arity_error::expected_arity()) — useful if you want to surface a
// custom UI message rather than the default formatted string.

#include <numsim_cas/parser/parse_error.h>
#include <numsim_cas/parser/parser.h>
#include <numsim_cas/parser/symbol_table.h>

#include <iostream>
#include <string_view>

namespace cas = numsim::cas;

// Print a horizontal rule for readability.
static void rule() { std::cout << "-----\n"; }

// Try to parse; catch each specific subclass and print structured
// info plus the rendered what(). Falls back to parse_error for any
// subclass we don't handle explicitly.
static void try_parse(std::string_view source) {
  std::cout << "input: " << source << "\n";
  cas::parser::symbol_table syms;
  try {
    [[maybe_unused]] auto e = cas::parser::parse(source, syms);
    std::cout << "  (parsed successfully)\n";
  } catch (cas::parser::arity_error const &e) {
    std::cout << "  arity_error\n";
    std::cout << "    function       = " << e.function() << "\n";
    std::cout << "    expected_arity = " << e.expected_arity() << "\n";
    std::cout << "    actual_arity   = " << e.actual_arity() << "\n";
    std::cout << e.what() << "\n";
  } catch (cas::parser::unknown_function_error const &e) {
    std::cout << "  unknown_function_error\n";
    std::cout << "    name = " << e.name() << "\n";
    std::cout << e.what() << "\n";
  } catch (cas::parser::lexical_error const &e) {
    std::cout << "  lexical_error\n" << e.what() << "\n";
  } catch (cas::parser::type_mismatch_error const &e) {
    std::cout << "  type_mismatch_error\n" << e.what() << "\n";
  } catch (cas::parser::redeclaration_error const &e) {
    std::cout << "  redeclaration_error\n" << e.what() << "\n";
  } catch (cas::parser::type_collision_error const &e) {
    std::cout << "  type_collision_error\n" << e.what() << "\n";
  } catch (cas::parser::syntax_error const &e) {
    std::cout << "  syntax_error\n" << e.what() << "\n";
  } catch (cas::parser::parse_error const &e) {
    std::cout << "  parse_error (base)\n" << e.what() << "\n";
  }
  std::cout << "\n";
}

int main() {
  std::cout << "=== Parser: Error Handling ===\n\n";

  try_parse("sin(x)");
  rule();
  try_parse("sin(x, y)"); // arity
  rule();
  try_parse("no_such_fn(x)"); // unknown function
  rule();
  try_parse("trans(x)"); // type mismatch — trans wants tensor
  rule();
  try_parse("sin(x"); // missing close paren
  rule();
  try_parse("trace(A{rank=0, dim=3})"); // zero rank from action
  rule();
  try_parse("dot_product(A{rank=2, dim=3}, [0, 1], "
            "B{rank=2, dim=3}, [1, 2])"); // lexical: zero index
  rule();
  try_parse("x + x{rank=2, dim=3}"); // scalar then tensor — collision
  rule();
  try_parse("trace(A{rank=2, dim=3}) + "
            "dot(A{rank=4, dim=3})"); // shape mismatch in one parse

  return 0;
}
