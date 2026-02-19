#ifndef PROJECTOR_ALGEBRA_H
#define PROJECTOR_ALGEBRA_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/tensor/functions/inner_product_wrapper.h>
#include <numsim_cas/tensor/projection_tensor.h>
#include <cassert>
#include <optional>

namespace numsim::cas {

// ─── Projector space identification ─────────────────────────────────────────

enum class ProjKind { Sym, Skew, Vol, Dev, Other };

inline ProjKind classify(tensor_projector const &p) {
  auto const &sp = p.space();
  if (std::holds_alternative<Symmetric>(sp.perm)) {
    if (std::holds_alternative<AnyTraceTag>(sp.trace))
      return ProjKind::Sym;
    if (std::holds_alternative<VolumetricTag>(sp.trace))
      return ProjKind::Vol;
    if (std::holds_alternative<DeviatoricTag>(sp.trace))
      return ProjKind::Dev;
  }
  if (std::holds_alternative<Skew>(sp.perm) &&
      std::holds_alternative<AnyTraceTag>(sp.trace))
    return ProjKind::Skew;
  return ProjKind::Other;
}

// ─── Helper: detect P:A pattern ─────────────────────────────────────────────

struct projector_contraction_info {
  tensor_projector const *proj;
  expression_holder<tensor_expression> argument;
};

/// If expr is inner_product(tensor_projector, {3,4}, X, {1,2}), extract
/// the projector and argument X.
inline std::optional<projector_contraction_info>
as_projector_contraction(expression_holder<tensor_expression> const &expr) {
  if (!is_same<inner_product_wrapper>(expr))
    return std::nullopt;

  auto const &ip = expr.template get<inner_product_wrapper>();
  if (ip.indices_lhs() != sequence{3, 4} ||
      ip.indices_rhs() != sequence{1, 2})
    return std::nullopt;

  if (!is_same<tensor_projector>(ip.expr_lhs()))
    return std::nullopt;

  auto const &proj = ip.expr_lhs().template get<tensor_projector>();
  if (proj.acts_on_rank() != 2)
    return std::nullopt;

  return projector_contraction_info{&proj, ip.expr_rhs()};
}

// ─── Projector algebra tables ───────────────────────────────────────────────

enum class ContractionRule { Idempotent, Zero, LhsSubspace, RhsSubspace };

inline std::optional<ContractionRule> contraction_rule(ProjKind lhs,
                                                       ProjKind rhs) {
  if (lhs == rhs)
    return ContractionRule::Idempotent;

  // Orthogonal pairs
  if ((lhs == ProjKind::Vol && rhs == ProjKind::Dev) ||
      (lhs == ProjKind::Dev && rhs == ProjKind::Vol))
    return ContractionRule::Zero;
  if ((lhs == ProjKind::Sym && rhs == ProjKind::Skew) ||
      (lhs == ProjKind::Skew && rhs == ProjKind::Sym))
    return ContractionRule::Zero;

  // Subspace relations: Vol ⊂ Sym, Dev ⊂ Sym
  if (lhs == ProjKind::Vol && rhs == ProjKind::Sym)
    return ContractionRule::LhsSubspace;
  if (lhs == ProjKind::Dev && rhs == ProjKind::Sym)
    return ContractionRule::LhsSubspace;
  if (lhs == ProjKind::Sym && rhs == ProjKind::Vol)
    return ContractionRule::RhsSubspace;
  if (lhs == ProjKind::Sym && rhs == ProjKind::Dev)
    return ContractionRule::RhsSubspace;

  return std::nullopt;
}

/// Try to combine two projectors via addition.
/// Returns the combined ProjKind or std::nullopt.
inline std::optional<ProjKind> addition_rule(ProjKind a, ProjKind b) {
  // Vol + Dev = Sym
  if ((a == ProjKind::Vol && b == ProjKind::Dev) ||
      (a == ProjKind::Dev && b == ProjKind::Vol))
    return ProjKind::Sym;

  // Sym + Skew = identity (not a projector - handled specially)
  // We return std::nullopt and handle it in the caller.
  return std::nullopt;
}

/// Check if Sym + Skew → identity
inline bool is_identity_sum(ProjKind a, ProjKind b) {
  return (a == ProjKind::Sym && b == ProjKind::Skew) ||
         (a == ProjKind::Skew && b == ProjKind::Sym);
}

// ─── Construction-time contraction check ─────────────────────────────────────

struct contraction_simplification {
  ContractionRule rule;
  ProjKind result_kind; // the resulting projector kind (for non-zero rules)
  expression_holder<tensor_expression> argument; // the inner argument X
  std::size_t dim;
};

/// Given an outer projector kind and an expression, check if the expression
/// is itself a projector contraction P_inner:X and return the simplification.
inline std::optional<contraction_simplification>
try_simplify_projector_contraction(
    ProjKind outer,
    expression_holder<tensor_expression> const &expr) {
  if (outer == ProjKind::Other)
    return std::nullopt;

  auto inner = as_projector_contraction(expr);
  if (!inner)
    return std::nullopt;

  auto k_inner = classify(*inner->proj);
  if (k_inner == ProjKind::Other)
    return std::nullopt;

  auto rule = contraction_rule(outer, k_inner);
  if (!rule)
    return std::nullopt;

  ProjKind result = ProjKind::Other;
  switch (*rule) {
  case ContractionRule::Idempotent:  result = outer;   break;
  case ContractionRule::Zero:        break;
  case ContractionRule::LhsSubspace: result = outer;   break;
  case ContractionRule::RhsSubspace: result = k_inner; break;
  }

  return contraction_simplification{*rule, result, inner->argument,
                                    inner->proj->dim()};
}

// ─── Reconstruct projection from ProjKind ────────────────────────────────────

inline expression_holder<tensor_expression>
apply_projection(ProjKind kind,
                 expression_holder<tensor_expression> const &arg) {
  auto d = arg.get().dim();
  switch (kind) {
  case ProjKind::Dev:
    return make_expression<inner_product_wrapper>(
        P_devi(d), sequence{3, 4}, arg, sequence{1, 2});
  case ProjKind::Sym:
    return make_expression<inner_product_wrapper>(
        P_sym(d), sequence{3, 4}, arg, sequence{1, 2});
  case ProjKind::Vol:
    return make_expression<inner_product_wrapper>(
        P_vol(d), sequence{3, 4}, arg, sequence{1, 2});
  case ProjKind::Skew:
    return make_expression<inner_product_wrapper>(
        P_skew(d), sequence{3, 4}, arg, sequence{1, 2});
  default:
    assert(false && "apply_projection: invalid ProjKind");
    return {};
  }
}

} // namespace numsim::cas

#endif // PROJECTOR_ALGEBRA_H
