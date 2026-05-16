#ifndef NUMSIM_CAS_TENSOR_SKEW_CLASSIFICATION_H
#define NUMSIM_CAS_TENSOR_SKEW_CLASSIFICATION_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/tensor/functions/inner_product_wrapper.h>
#include <numsim_cas/tensor/functions/permute_indices.h>
#include <numsim_cas/tensor/operators/scalar/tensor_scalar_mul.h>
#include <numsim_cas/tensor/operators/tensor/tensor_add.h>
#include <numsim_cas/tensor/operators/tensor/tensor_mul.h>
#include <numsim_cas/tensor/projection_tensor.h>
#include <numsim_cas/tensor/sequence.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_negative.h>
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
  // inner_product(P_skew, {3,4}, X, {1,2}) — the skew() projection
  if (is_same<inner_product_wrapper>(e)) {
    auto const &ip = e.template get<inner_product_wrapper>();
    if (is_same<tensor_projector>(ip.expr_lhs())) {
      auto const &proj = ip.expr_lhs().template get<tensor_projector>();
      if (std::holds_alternative<Skew>(proj.space().perm))
        return true;
    }
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

// Walks compound expressions to check whether any factor is provably skew.
// A scalar multiple or negation preserves singularity; tensor_mul and
// inner_product are singular if any factor is. The walk into tensor_mul /
// inner_product recurses via contains_skew_factor so a scaled or negated skew
// child (e.g. tensor_mul(2*skew(A), B)) is still caught.
[[nodiscard]] inline bool
contains_skew_factor(expression_holder<tensor_expression> const &e) {
  if (!e.is_valid())
    return false;
  if (is_provably_skew(e))
    return true;
  if (is_same<tensor_mul>(e)) {
    for (auto const &child : e.template get<tensor_mul>().data()) {
      if (contains_skew_factor(child))
        return true;
    }
  }
  if (is_same<inner_product_wrapper>(e)) {
    auto const &ip = e.template get<inner_product_wrapper>();
    if (contains_skew_factor(ip.expr_lhs()) ||
        contains_skew_factor(ip.expr_rhs()))
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
