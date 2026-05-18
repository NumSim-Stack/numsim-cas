#ifndef SCALAR_COMPARISON_PRINT_HELPER_H
#define SCALAR_COMPARISON_PRINT_HELPER_H

// Shared infix-printing helper for the six scalar comparison nodes (#136).
//
// Both `scalar_printer` and `scalar_latex_printer` emit the same shape —
// `<open> lhs op rhs <close>` — differing only in bracket strings ("(" vs
// "\left(") and operator glyphs ("<=" vs "\le", etc.). Centralising the
// loop here keeps the per-printer translation units to a single line per
// node.

namespace numsim::cas::detail {

template <typename Stream, typename Node, typename ApplyFn>
inline void print_infix_comparison(Stream &out, Node const &v, const char *open,
                                   const char *op, const char *close,
                                   ApplyFn apply) {
  out << open;
  apply(v.expr_lhs());
  out << " " << op << " ";
  apply(v.expr_rhs());
  out << close;
}

} // namespace numsim::cas::detail

#endif // SCALAR_COMPARISON_PRINT_HELPER_H
