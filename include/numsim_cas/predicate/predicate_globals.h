#ifndef PREDICATE_GLOBALS_H
#define PREDICATE_GLOBALS_H

#include <numsim_cas/predicate/predicate_expression.h>

namespace numsim::cas {

// Cached `true` / `false` leaf expression_holders. All simplifications that
// fold a comparison to a literal return one of these — there's never any
// need for a fresh make_expression<predicate_true>() call inside the
// simplifier or evaluator paths.
expression_holder<predicate_expression> const &get_predicate_true() noexcept;
expression_holder<predicate_expression> const &get_predicate_false() noexcept;

} // namespace numsim::cas

#endif // PREDICATE_GLOBALS_H
