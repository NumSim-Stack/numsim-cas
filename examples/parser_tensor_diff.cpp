// Parser ↔ rest-of-library — parse a tensor-to-scalar expression,
// differentiate it w.r.t. the parser-declared tensor variable, print
// the result.
//
// Shows that parser output is a normal `expression_holder` that the
// existing symbolic algebra (diff, simplification, printing) accepts
// transparently — there's nothing parser-specific downstream.

#include <numsim_cas/numsim_cas.h>
// The umbrella header above doesn't currently pull in the
// tensor-to-scalar diff routing; include it explicitly so
// `cas::diff(t2s_expr, tensor_holder)` resolves.
#include <numsim_cas/parser/parser.h>
#include <numsim_cas/parser/symbol_table.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_diff.h>

#include <iostream>

int main() {
  namespace cas = numsim::cas;

  std::cout << "=== Parser: Tensor Differentiation ===\n\n";

  // f(A) = trace(A) — d/dA = rank-2 identity tensor I.
  {
    cas::parser::symbol_table syms;
    auto f = cas::parser::parse_t2s("trace(A{rank=2, dim=3})", syms);
    auto A = cas::parser::parse_tensor("A", syms);
    auto df = cas::diff(f, A);
    std::cout << "f(A)   = " << cas::to_string(f) << "\n";
    std::cout << "df/dA  = " << cas::to_string(df) << "  (rank-2 identity)\n\n";
  }

  // g(A) = trace(A) * trace(A) = (tr A)^2 — d/dA = 2 * trace(A) * I.
  {
    cas::parser::symbol_table syms;
    auto g = cas::parser::parse_t2s("trace(A{rank=2, dim=3}) * trace(A)", syms);
    auto A = cas::parser::parse_tensor("A", syms);
    auto dg = cas::diff(g, A);
    std::cout << "g(A)   = " << cas::to_string(g) << "\n";
    std::cout << "dg/dA  = " << cas::to_string(dg) << "\n\n";
  }

  // h(A) = norm(A) — d/dA = A / norm(A).
  {
    cas::parser::symbol_table syms;
    auto h = cas::parser::parse_t2s("norm(A{rank=2, dim=3})", syms);
    auto A = cas::parser::parse_tensor("A", syms);
    auto dh = cas::diff(h, A);
    std::cout << "h(A)   = " << cas::to_string(h) << "\n";
    std::cout << "dh/dA  = " << cas::to_string(dh) << "\n";
  }

  return 0;
}
