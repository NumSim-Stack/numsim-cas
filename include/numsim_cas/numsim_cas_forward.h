#ifndef NUMSIM_CAS_FORWARD_H
#define NUMSIM_CAS_FORWARD_H

namespace numsim::cas {

template <typename BaseExpr> class expression_holder;
class scalar_expression;
class tensor_expression;
class tensor_to_scalar_expression;
class tensor;
class scalar_number;
template <typename BaseExpr> class symbol_base;
template <typename BaseExpr> class n_ary_tree;
template <typename ThisBaseExpr, typename BaseExpr> class unary_op;
template <typename ThisBaseExpr, typename BaseExprLHS, typename BaseExprRHS>
class binary_op;

template <typename ExprLHS, typename ExprRHS> class compare_less_operator;
} // namespace numsim::cas

#endif // NUMSIM_CAS_FORWARD_H
