#ifndef TENSOR_TO_SCALAR_VISITOR_TYPEDEF_H
#define TENSOR_TO_SCALAR_VISITOR_TYPEDEF_H

#include <cstdint>
#include <numsim_cas/core/visitor_base.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_node_list.h>
#include <numsim_cas/type_list.h>

namespace numsim::cas {

// Base expression type
class tensor_expression;

// Forward declare nodes using the list
#define NUMSIM_CAS_FWD_FIRST(T) class T;
#define NUMSIM_CAS_FWD_NEXT(T) class T;
NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_CAS_FWD_FIRST, NUMSIM_CAS_FWD_NEXT)
#undef NUMSIM_CAS_FWD_FIRST
#undef NUMSIM_CAS_FWD_NEXT

// Build typelist of nodes
#define NUMSIM_CAS_TYPELIST_FIRST(T) T
#define NUMSIM_CAS_TYPELIST_NEXT(T) , T
using tensor_to_scalar_node_types =
    type_list<NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_CAS_TYPELIST_FIRST,
                                                    NUMSIM_CAS_TYPELIST_NEXT)>;
#undef NUMSIM_CAS_TYPELIST_FIRST
#undef NUMSIM_CAS_TYPELIST_NEXT

template <typename T> struct get_index<T, tensor_to_scalar_expression> {
  static constexpr std::uint16_t index =
      index_of_v<T, tensor_to_scalar_node_types>;
};

// Visitor typedefs
#define NUMSIM_CAS_VIS_FIRST(T) T
#define NUMSIM_CAS_VIS_NEXT(T) , T
template <typename ReturnType>
using tensor_to_scalar_visitor_return_t =
    visitor_return<ReturnType, NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(
                                   NUMSIM_CAS_VIS_FIRST, NUMSIM_CAS_VIS_NEXT)>;
#undef NUMSIM_CAS_VIS_FIRST
#undef NUMSIM_CAS_VIS_NEXT

using tensor_to_scalar_visitor_return_expr_t =
    tensor_to_scalar_visitor_return_t<
        expression_holder<tensor_to_scalar_expression>>;

#define NUMSIM_CAS_VIS_FIRST(T) T
#define NUMSIM_CAS_VIS_NEXT(T) , T
using tensor_to_scalar_visitor_t =
    visitor<NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_CAS_VIS_FIRST,
                                                  NUMSIM_CAS_VIS_NEXT)>;
using tensor_to_scalar_visitor_const_t =
    visitor_const<NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_CAS_VIS_FIRST,
                                                        NUMSIM_CAS_VIS_NEXT)>;
#undef NUMSIM_CAS_VIS_FIRST
#undef NUMSIM_CAS_VIS_NEXT

// Visitable base typedef
#define NUMSIM_CAS_VT_FIRST(T) T
#define NUMSIM_CAS_VT_NEXT(T) , T
using tensor_to_scalar_visitable_t =
    visitable<tensor_to_scalar_expression,
              NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_CAS_VT_FIRST,
                                                    NUMSIM_CAS_VT_NEXT)>;
#undef NUMSIM_CAS_VT_FIRST
#undef NUMSIM_CAS_VT_NEXT

// Node base alias for each concrete node T
#define NUMSIM_CAS_NB_FIRST(T) T
#define NUMSIM_CAS_NB_NEXT(T) , T
template <typename T>
using tensor_to_scalar_node_base_t =
    visitable_impl<tensor_to_scalar_expression, T,
                   NUMSIM_CAS_TENSOR_TO_SCALAR_NODE_LIST(NUMSIM_CAS_NB_FIRST,
                                                         NUMSIM_CAS_NB_NEXT)>;
#undef NUMSIM_CAS_NB_FIRST
#undef NUMSIM_CAS_NB_NEXT

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_VISITOR_TYPEDEF_H
