// Parser symbol_table lifecycle — the REPL-style usage pattern.
//
// One symbol_table threaded through multiple parse() calls. Tensors
// declared once via `Name{rank=R, dim=D}` can then be referenced
// bare in later expressions (lookup-first identifier resolution).
// Scalars are implicitly declared on first bare use.

#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/parser/parse_error.h>
#include <numsim_cas/parser/parser.h>
#include <numsim_cas/parser/symbol_table.h>

#include <iostream>

int main() {
  namespace cas = numsim::cas;

  std::cout << "=== Parser: Symbol Table Lifecycle ===\n\n";

  cas::parser::symbol_table syms;

  // (1) Declare A as a rank-2 tensor on the right of a t2s function.
  auto e1 = cas::parser::parse_t2s("trace(A{rank=2, dim=3})", syms);
  std::cout << "1. parsed: " << cas::to_string(e1) << "\n";
  auto shape = syms.tensor_shape("A");
  std::cout << "   A registered as rank=" << shape->first
            << ", dim=" << shape->second << "\n\n";

  // (2) Reuse A bare — same holder resolves through symbol_table.
  auto e2 = cas::parser::parse_t2s("trace(A) + det(A)", syms);
  std::cout << "2. parsed: " << cas::to_string(e2) << "\n\n";

  // (3) Mix declared tensor with implicit scalar in one expression.
  //     `lambda` is bare → implicitly declared as scalar.
  auto e3 = cas::parser::parse_t2s("lambda * trace(A)", syms);
  std::cout << "3. parsed: " << cas::to_string(e3) << "\n";
  std::cout << "   lambda now declared as scalar: " << std::boolalpha
            << syms.has("lambda") << "\n\n";

  // (4) Identical re-declaration is allowed — same (rank, dim).
  auto e4 = cas::parser::parse_t2s("det(A{rank=2, dim=3})", syms);
  std::cout << "4. parsed: " << cas::to_string(e4)
            << "  (identical re-declaration OK)\n\n";

  // (5) Re-declaring with a DIFFERENT shape throws redeclaration_error.
  //     Catch and demonstrate the diagnostic.
  std::cout << "5. attempting A{rank=4, dim=3} (mismatched shape)...\n";
  try {
    [[maybe_unused]] auto e5 =
        cas::parser::parse_tensor("A{rank=4, dim=3}", syms);
  } catch (cas::parser::redeclaration_error const &e) {
    std::cout << "   threw redeclaration_error:\n";
    std::cout << "   " << e.what() << "\n";
  }

  return 0;
}
