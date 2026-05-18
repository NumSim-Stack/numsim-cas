#ifndef PREDICATE_DEFINITIONS_H
#define PREDICATE_DEFINITIONS_H

// Umbrella include for the predicate domain. Pulls in the base class, the
// node list / visitor typedefs, the two leaf nodes, and the global-access
// helpers. Subsequent PRs (#136 comparisons, #135 if_then_else) will add
// further headers and append them here.

#include "predicate_expression.h"
#include "predicate_false.h"
#include "predicate_globals.h"
#include "predicate_io.h"
#include "predicate_node_list.h"
#include "predicate_true.h"
#include "predicate_visitor_typedef.h"

#endif // PREDICATE_DEFINITIONS_H
