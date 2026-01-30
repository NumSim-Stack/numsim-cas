#ifndef CORE_FWD_H
#define CORE_FWD_H

namespace numsim::cas {
template <typename ThisBase, typename BaseLHS, typename BaseRHS>
class binary_op;
template <typename ExprBase> class expression_holder;
class expression;
template <typename Base> class n_ary_tree;
template <typename Base> class n_ary_vector;
class scalar_number;
template <typename BaseExpr> class symbol_base;
template <typename ThisBase, typename ExprBase> class unary_op;
} // namespace numsim::cas
#endif // CORE_FWD_H
