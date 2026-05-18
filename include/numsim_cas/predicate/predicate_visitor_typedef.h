#ifndef PREDICATE_VISITOR_TYPEDEF_H
#define PREDICATE_VISITOR_TYPEDEF_H

#include <numsim_cas/core/visitor_base.h>
#include <numsim_cas/predicate/predicate_node_list.h>
#include <numsim_cas/type_list.h>

#include <cstdint>

namespace numsim::cas {

class predicate_expression;

// Forward declare every node in the predicate domain.
#define NUMSIM_CAS_FWD_FIRST(T) class T;
#define NUMSIM_CAS_FWD_NEXT(T) class T;
NUMSIM_CAS_PREDICATE_NODE_LIST(NUMSIM_CAS_FWD_FIRST, NUMSIM_CAS_FWD_NEXT)
#undef NUMSIM_CAS_FWD_FIRST
#undef NUMSIM_CAS_FWD_NEXT

// Typelist of nodes (for get_index lookup).
#define NUMSIM_CAS_TYPELIST_FIRST(T) T
#define NUMSIM_CAS_TYPELIST_NEXT(T) , T
using predicate_node_types = type_list<NUMSIM_CAS_PREDICATE_NODE_LIST(
    NUMSIM_CAS_TYPELIST_FIRST, NUMSIM_CAS_TYPELIST_NEXT)>;
#undef NUMSIM_CAS_TYPELIST_FIRST
#undef NUMSIM_CAS_TYPELIST_NEXT

template <typename T> struct get_index<T, predicate_expression> {
  static constexpr std::uint16_t index = index_of_v<T, predicate_node_types>;
};

// Visitor typedefs.
#define NUMSIM_CAS_VIS_FIRST(T) T
#define NUMSIM_CAS_VIS_NEXT(T) , T
template <typename ReturnType>
using predicate_visitor_return_t =
    visitor_return<ReturnType, NUMSIM_CAS_PREDICATE_NODE_LIST(
                                   NUMSIM_CAS_VIS_FIRST, NUMSIM_CAS_VIS_NEXT)>;
#undef NUMSIM_CAS_VIS_FIRST
#undef NUMSIM_CAS_VIS_NEXT

using predicate_visitor_return_expr_t =
    predicate_visitor_return_t<expression_holder<predicate_expression>>;

#define NUMSIM_CAS_VIS_FIRST(T) T
#define NUMSIM_CAS_VIS_NEXT(T) , T
using predicate_visitor_t = visitor<NUMSIM_CAS_PREDICATE_NODE_LIST(
    NUMSIM_CAS_VIS_FIRST, NUMSIM_CAS_VIS_NEXT)>;
using predicate_visitor_const_t = visitor_const<NUMSIM_CAS_PREDICATE_NODE_LIST(
    NUMSIM_CAS_VIS_FIRST, NUMSIM_CAS_VIS_NEXT)>;
#undef NUMSIM_CAS_VIS_FIRST
#undef NUMSIM_CAS_VIS_NEXT

// Visitable base typedef.
#define NUMSIM_CAS_VT_FIRST(T) T
#define NUMSIM_CAS_VT_NEXT(T) , T
using predicate_visitable_t =
    visitable<predicate_expression,
              NUMSIM_CAS_PREDICATE_NODE_LIST(NUMSIM_CAS_VT_FIRST,
                                             NUMSIM_CAS_VT_NEXT)>;
#undef NUMSIM_CAS_VT_FIRST
#undef NUMSIM_CAS_VT_NEXT

// Node base alias for each concrete node T.
#define NUMSIM_CAS_NB_FIRST(T) T
#define NUMSIM_CAS_NB_NEXT(T) , T
template <typename T>
using predicate_node_base_t =
    visitable_impl<predicate_expression, T,
                   NUMSIM_CAS_PREDICATE_NODE_LIST(NUMSIM_CAS_NB_FIRST,
                                                  NUMSIM_CAS_NB_NEXT)>;
#undef NUMSIM_CAS_NB_FIRST
#undef NUMSIM_CAS_NB_NEXT

} // namespace numsim::cas

#endif // PREDICATE_VISITOR_TYPEDEF_H
