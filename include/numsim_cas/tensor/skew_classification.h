#ifndef NUMSIM_CAS_TENSOR_SKEW_CLASSIFICATION_H
#define NUMSIM_CAS_TENSOR_SKEW_CLASSIFICATION_H

#include <algorithm>
#include <numsim_cas/basic_functions.h>
#include <numsim_cas/tensor/operators/scalar/tensor_scalar_mul.h>
#include <numsim_cas/tensor/operators/tensor/tensor_add.h>
#include <numsim_cas/tensor/operators/tensor/tensor_mul.h>
#include <numsim_cas/tensor/projection_tensor.h>
#include <numsim_cas/tensor/projector_algebra.h>
#include <numsim_cas/tensor/sequence.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_negative.h>
#include <numsim_cas/tensor/wrappers/inner_product_wrapper.h>
#include <numsim_cas/tensor/wrappers/permute_indices_wrapper.h>
#include <variant>

namespace numsim::cas {

// Structural classifier for skew-symmetric rank-2 tensor expressions.
//
// Returns true when the expression can be proven skew-symmetric by inspecting
// its structure, without needing to evaluate it. Used to reject mathematically
// invalid constructs at expression-build time (e.g. inv() of a skew tensor in
// odd dimensions, which is singular by determinant theorem).
//
// Why structural detection, not just space annotation:
// `tensor_space` annotations (Skew / Symmetric / etc.) are not reliably
// preserved across all composition patterns — in particular, an annotation on
// a child of `tensor_mul` does not propagate to the parent node, and some
// rewrite paths drop annotations entirely. Structural pattern matching is the
// authoritative check; the space annotation is a fast path.
[[nodiscard]] inline bool
is_provably_skew(expression_holder<tensor_expression> const &e) {
  if (!e.is_valid())
    return false;
  if (auto const &sp = e.get().space()) {
    if (std::holds_alternative<Skew>(sp->perm))
      return true;
  }
  // inner_product(P_skew, {3,4}, X, {1,2}) with rank-2 X — the skew()
  // projection. Other contraction patterns of P_skew need not be skew.
  if (auto pc = as_projector_contraction(e)) {
    if (std::holds_alternative<Skew>(pc->proj->space().perm) &&
        pc->argument.get().rank() == 2)
      return true;
  }
  // tensor_add with exactly two children: trans(X) + (-X)
  if (is_same<tensor_add>(e)) {
    auto const &add = e.template get<tensor_add>();
    if (add.symbol_map().size() == 2) {
      auto it = add.symbol_map().begin();
      auto const &c0 = it->second;
      ++it;
      auto const &c1 = it->second;
      auto trans_of_neg = [](auto const &a, auto const &b) {
        if (!is_same<permute_indices_wrapper>(a))
          return false;
        auto const &bc = a.template get<permute_indices_wrapper>();
        if (bc.indices() != sequence{2, 1})
          return false;
        if (!is_same<tensor_negative>(b))
          return false;
        return bc.expr() == b.template get<tensor_negative>().expr();
      };
      if (trans_of_neg(c0, c1) || trans_of_neg(c1, c0))
        return true;
    }
  }
  return false;
}

// Checks whether a rank-2 expression is a matrix product with a provably skew
// (hence, in odd dimensions, singular) factor. det(M1·M2) = det(M1)·det(M2)
// only holds for products of rank-2 matrices, so the walk recurses through
// tensor_mul chains of rank-2 children and single-index contractions of two
// rank-2 operands (a product of the operands or their transposes), plus
// scalar multiples and negations.
[[nodiscard]] inline bool
contains_skew_factor(expression_holder<tensor_expression> const &e) {
  if (!e.is_valid())
    return false;
  if (is_provably_skew(e))
    return true;
  if (is_same<tensor_mul>(e)) {
    auto const &children = e.template get<tensor_mul>().data();
    bool const matrix_chain =
        std::all_of(children.begin(), children.end(),
                    [](auto const &child) { return child.get().rank() == 2; });
    if (matrix_chain) {
      for (auto const &child : children) {
        if (contains_skew_factor(child))
          return true;
      }
    }
  }
  if (is_same<inner_product_wrapper>(e)) {
    auto const &ip = e.template get<inner_product_wrapper>();
    if (ip.expr_lhs().get().rank() == 2 && ip.expr_rhs().get().rank() == 2 &&
        ip.indices_lhs().size() == 1 && ip.indices_rhs().size() == 1 &&
        (contains_skew_factor(ip.expr_lhs()) ||
         contains_skew_factor(ip.expr_rhs())))
      return true;
  }
  if (is_same<tensor_scalar_mul>(e)) {
    if (contains_skew_factor(e.template get<tensor_scalar_mul>().expr_rhs()))
      return true;
  }
  if (is_same<tensor_negative>(e)) {
    if (contains_skew_factor(e.template get<tensor_negative>().expr()))
      return true;
  }
  return false;
}

} // namespace numsim::cas

#endif // NUMSIM_CAS_TENSOR_SKEW_CLASSIFICATION_H
