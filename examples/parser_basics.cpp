// Parser basics — parse a scalar expression, declare variable bindings
// through the symbol_table the parser populated, and evaluate.
//
// Include-hygiene: this file uses only the PUBLIC parser headers
// (`<numsim_cas/parser/*.h>`). It deliberately does NOT include any
// PEGTL header. If it builds, the public-headers-don't-leak-PEGTL
// constraint holds — internal grammar template instantiations stay
// inside `NumSim_CAS_Parser`'s translation units.

#include <numsim_cas/numsim_cas.h>
#include <numsim_cas/parser/parser.h>
#include <numsim_cas/parser/symbol_table.h>
#include <numsim_cas/scalar/visitors/scalar_evaluator.h>

#include <iostream>

int main() {
  namespace cas = numsim::cas;

  std::cout << "=== Parser Basics ===\n\n";

  // Parse a quadratic; implicitly declares a, b, c, x as scalars.
  cas::parser::symbol_table syms;
  auto e = cas::parser::parse_scalar("a*x^2 + b*x + c", syms);
  std::cout << "parsed: " << cas::to_string(e) << "\n\n";

  // Evaluate (a=1, b=-3, c=2) at several x; the roots of x^2 - 3x + 2
  // are 1 and 2.
  cas::scalar_evaluator<double> ev;
  ev.set(syms.get_or_declare_scalar("a"), 1.0);
  ev.set(syms.get_or_declare_scalar("b"), -3.0);
  ev.set(syms.get_or_declare_scalar("c"), 2.0);

  std::cout << "f(x) = x^2 - 3x + 2 sampled:\n";
  for (double x : {0.0, 1.0, 1.5, 2.0, 3.0}) {
    ev.set(syms.get_or_declare_scalar("x"), x);
    std::cout << "  f(" << x << ") = " << ev.apply(e) << "\n";
  }

  return 0;
}
