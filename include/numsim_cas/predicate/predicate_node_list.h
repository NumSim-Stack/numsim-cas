#ifndef PREDICATE_NODE_LIST_H
#define PREDICATE_NODE_LIST_H

// X-macro registry of every node type in the predicate domain.
// Predicates carry a boolean value (true / false). Inhabitants of the
// domain are the two literal leaves plus (in later PRs) comparison nodes
// produced by scalar `<`/`>`/`==` operators and logical combinators
// (`and`/`or`/`not`).
//
// Usage: NUMSIM_CAS_PREDICATE_NODE_LIST(FIRST_MACRO, NEXT_MACRO)
// FIRST_MACRO(T) expands the first element (no leading comma).
// NEXT_MACRO(T) expands subsequent elements (with leading comma).

#define NUMSIM_CAS_PREDICATE_NODE_LIST(FIRST, NEXT)                            \
  FIRST(predicate_true)                                                        \
  NEXT(predicate_false)

#endif // PREDICATE_NODE_LIST_H
