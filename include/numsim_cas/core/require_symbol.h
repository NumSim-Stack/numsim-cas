#ifndef REQUIRE_SYMBOL_H
#define REQUIRE_SYMBOL_H

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/expression.h>
#include <string>
#include <string_view>

namespace numsim::cas::detail {

// SymPy-style guard for assume_* helpers. Only Symbols (named variables
// like tensor("A", 3, 2) or scalar("x")) can carry user-asserted facts.
// Compounds (A + B, inv(A), trans(A)) derive their facts from structure;
// constants (identity_tensor, tensor_zero, tensor_projector) derive from
// their type. Asserting facts on either is a category error — the user
// is conflating "structural property of this expression" (already known
// or derivable) with "fact about the value the user has in mind"
// (only meaningful for Symbols).
//
// Throws invalid_assumption_error if expr is not a Symbol. Callers pass
// the helper's name for a useful diagnostic. expression::is_symbol() is
// virtual; symbol_base overrides it to true, the default is false. The
// t2s scalar wrapper forwards through to its inner scalar's answer so
// wrapped scalar Symbols remain assumable.
inline void require_symbol(expression const &expr, std::string_view fn_name) {
  if (!expr.is_symbol())
    throw invalid_assumption_error(
        std::string(fn_name) +
        ": requires a Symbol (named variable); compounds and constants "
        "cannot carry user-asserted facts in the SymPy assumption model");
}

} // namespace numsim::cas::detail

#endif // REQUIRE_SYMBOL_H
