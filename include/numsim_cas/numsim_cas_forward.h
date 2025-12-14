#ifndef NUMSIM_CAS_FORWARD_H
#define NUMSIM_CAS_FORWARD_H

namespace numsim::cas {

template <typename ExprBase> class expression_holder;
template <typename ValueType> class scalar_expression;
template <typename ValueType> class tensor_expression;
template <typename ValueType> class tensor;
template <typename BaseExpr, typename Derived> class symbol_base;
template <typename BaseExpr, typename Derived> class n_ary_tree;
template <typename Derived, typename Base, typename ExprType> class unary_op;
template <typename Derived, typename Base, typename BaseLHS, typename BaseRHS>
class binary_op;
} // namespace numsim::cas

#endif // NUMSIM_CAS_FORWARD_H
