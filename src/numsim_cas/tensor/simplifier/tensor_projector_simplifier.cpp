#include <numsim_cas/tensor/simplifier/tensor_projector_simplifier.h>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/tensor/projection_tensor.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <ranges>
#include <variant>

namespace numsim::cas {

namespace {

// ─── Helper: detect P:A pattern ─────────────────────────────────────────────

struct projector_contraction_info {
  tensor_projector const *proj;
  expression_holder<tensor_expression> argument;
};

/// If expr is inner_product(tensor_projector, {3,4}, X, {1,2}), extract
/// the projector and argument X.
std::optional<projector_contraction_info>
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

// ─── Projector space identification ─────────────────────────────────────────

enum class ProjKind { Sym, Skew, Vol, Dev, Other };

ProjKind classify(tensor_projector const &p) {
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

// ─── Projector algebra tables ───────────────────────────────────────────────

/// P : P contraction result.
/// Returns: 0 = same projector (idempotent), 1 = zero (orthogonal),
///          2 = lhs is subspace of rhs → return lhs,
///          3 = rhs is subspace of lhs → return rhs,
///         -1 = no simplification.
int contraction_rule(ProjKind lhs, ProjKind rhs) {
  if (lhs == rhs)
    return 0; // idempotent: P:P = P

  // Orthogonal pairs
  if ((lhs == ProjKind::Vol && rhs == ProjKind::Dev) ||
      (lhs == ProjKind::Dev && rhs == ProjKind::Vol))
    return 1; // zero
  if ((lhs == ProjKind::Sym && rhs == ProjKind::Skew) ||
      (lhs == ProjKind::Skew && rhs == ProjKind::Sym))
    return 1; // zero

  // Subspace relations: Vol ⊂ Sym, Dev ⊂ Sym
  if (lhs == ProjKind::Vol && rhs == ProjKind::Sym)
    return 2; // Vol:Sym = Vol → return lhs
  if (lhs == ProjKind::Dev && rhs == ProjKind::Sym)
    return 2; // Dev:Sym = Dev → return lhs
  if (lhs == ProjKind::Sym && rhs == ProjKind::Vol)
    return 3; // Sym:Vol = Vol → return rhs
  if (lhs == ProjKind::Sym && rhs == ProjKind::Dev)
    return 3; // Sym:Dev = Dev → return rhs

  return -1;
}

/// Try to combine two projectors via addition.
/// Returns the combined ProjKind or std::nullopt.
std::optional<ProjKind> addition_rule(ProjKind a, ProjKind b) {
  // Vol + Dev = Sym
  if ((a == ProjKind::Vol && b == ProjKind::Dev) ||
      (a == ProjKind::Dev && b == ProjKind::Vol))
    return ProjKind::Sym;

  // Sym + Skew = identity (not a projector - handled specially)
  // We return std::nullopt and handle it in the caller.
  return std::nullopt;
}

/// Check if Sym + Skew → identity
bool is_identity_sum(ProjKind a, ProjKind b) {
  return (a == ProjKind::Sym && b == ProjKind::Skew) ||
         (a == ProjKind::Skew && b == ProjKind::Sym);
}

/// Build P:A for a given ProjKind and dimension.
expression_holder<tensor_expression>
make_proj_contraction(ProjKind kind, std::size_t dim,
                      expression_holder<tensor_expression> const &arg) {
  switch (kind) {
  case ProjKind::Sym:
    return sym(arg);
  case ProjKind::Skew:
    return skew(arg);
  case ProjKind::Vol:
    return vol(arg);
  case ProjKind::Dev:
    return dev(arg);
  default:
    return {}; // should not happen
  }
}

} // namespace

// ─── Public API ─────────────────────────────────────────────────────────────

expression_holder<tensor_expression>
tensor_projector_simplifier::apply(tensor_holder_t const &expr) {
  return tensor_rebuild_visitor::apply(expr);
}

// ─── inner_product_wrapper: P:P contraction rules ───────────────────────────

void tensor_projector_simplifier::operator()(
    inner_product_wrapper const &v) {
  // First, recursively simplify both sides.
  auto lhs = apply(v.expr_lhs());
  auto rhs = apply(v.expr_rhs());

  // Check for P : (P : X) pattern
  if (v.indices_lhs() == sequence{3, 4} &&
      v.indices_rhs() == sequence{1, 2} &&
      is_same<tensor_projector>(lhs)) {
    auto const &proj_lhs = lhs.template get<tensor_projector>();
    if (proj_lhs.acts_on_rank() == 2) {
      auto inner_info = as_projector_contraction(rhs);
      if (inner_info) {
        auto k_lhs = classify(proj_lhs);
        auto k_rhs = classify(*inner_info->proj);
        if (k_lhs != ProjKind::Other && k_rhs != ProjKind::Other) {
          int rule = contraction_rule(k_lhs, k_rhs);
          auto dim = proj_lhs.dim();
          switch (rule) {
          case 0: // idempotent: P:P:X = P:X
            m_result = make_proj_contraction(k_lhs, dim, inner_info->argument);
            return;
          case 1: // orthogonal: P_i:P_j:X = 0
            m_result = make_expression<tensor_zero>(dim, v.rank());
            return;
          case 2: // lhs subspace: P_sub:P_par:X = P_sub:X
            m_result = make_proj_contraction(k_lhs, dim, inner_info->argument);
            return;
          case 3: // rhs subspace: P_par:P_sub:X = P_sub:X
            m_result = make_proj_contraction(k_rhs, dim, inner_info->argument);
            return;
          default:
            break;
          }
        }
      }

      // Also check: P : P (standalone projector contraction)
      if (is_same<tensor_projector>(rhs)) {
        auto const &proj_rhs = rhs.template get<tensor_projector>();
        if (proj_rhs.acts_on_rank() == 2) {
          auto k_lhs = classify(proj_lhs);
          auto k_rhs = classify(proj_rhs);
          if (k_lhs != ProjKind::Other && k_rhs != ProjKind::Other) {
            int rule = contraction_rule(k_lhs, k_rhs);
            switch (rule) {
            case 0: // P:P = P
              m_result = lhs;
              return;
            case 1: // orthogonal → zero (rank-4)
              m_result =
                  make_expression<tensor_zero>(proj_lhs.dim(), v.rank());
              return;
            case 2:
              m_result = lhs;
              return;
            case 3:
              m_result = rhs;
              return;
            default:
              break;
            }
          }
        }
      }
    }
  }

  // Default: rebuild the inner product
  m_result = make_expression<inner_product_wrapper>(
      std::move(lhs), v.indices_lhs(), std::move(rhs), v.indices_rhs());
}

// ─── tensor_add: factor common arguments and combine projectors ─────────────

void tensor_projector_simplifier::operator()(tensor_add const &v) {
  // First, recursively simplify all children.
  std::vector<tensor_holder_t> simplified_children;
  if (v.coeff().is_valid())
    simplified_children.push_back(apply(v.coeff()));
  for (auto const &child : v.hash_map() | std::views::values)
    simplified_children.push_back(apply(child));

  // Scan for projector contractions and group by argument hash.
  struct ProjEntry {
    ProjKind kind;
    std::size_t dim;
    expression_holder<tensor_expression> argument;
    std::size_t child_index;
  };

  // Map: argument hash → list of projector entries
  std::map<std::size_t, std::vector<ProjEntry>> groups;
  std::vector<bool> consumed(simplified_children.size(), false);

  for (std::size_t i = 0; i < simplified_children.size(); ++i) {
    auto info = as_projector_contraction(simplified_children[i]);
    if (!info)
      continue;
    auto k = classify(*info->proj);
    if (k == ProjKind::Other)
      continue;
    auto arg_hash = info->argument.get().hash_value();
    groups[arg_hash].push_back(
        {k, info->proj->dim(), info->argument, i});
  }

  // For each group, try to combine projectors.
  for (auto &[hash, entries] : groups) {
    if (entries.size() < 2)
      continue;

    // Try pairwise combinations.
    for (std::size_t i = 0; i < entries.size(); ++i) {
      if (consumed[entries[i].child_index])
        continue;
      for (std::size_t j = i + 1; j < entries.size(); ++j) {
        if (consumed[entries[j].child_index])
          continue;

        auto combined = addition_rule(entries[i].kind, entries[j].kind);
        if (combined) {
          // Replace entry i with the combined projector, mark j as consumed.
          consumed[entries[i].child_index] = true;
          consumed[entries[j].child_index] = true;
          auto new_expr = make_proj_contraction(
              *combined, entries[i].dim, entries[i].argument);
          simplified_children.push_back(std::move(new_expr));
          consumed.push_back(false);
          // Update the entry for further combination rounds.
          entries[i] = {*combined, entries[i].dim, entries[i].argument,
                        simplified_children.size() - 1};
          break;
        }

        if (is_identity_sum(entries[i].kind, entries[j].kind)) {
          // P_sym + P_skew = I → P_sym:A + P_skew:A = A
          consumed[entries[i].child_index] = true;
          consumed[entries[j].child_index] = true;
          simplified_children.push_back(entries[i].argument);
          consumed.push_back(false);
          break;
        }
      }
    }
  }

  // Rebuild the sum from non-consumed children.
  tensor_holder_t result;
  for (std::size_t i = 0; i < simplified_children.size(); ++i) {
    if (consumed[i])
      continue;
    if (simplified_children[i].is_valid())
      result += simplified_children[i];
  }
  m_result = result.is_valid()
                 ? std::move(result)
                 : make_expression<tensor_zero>(v.dim(), v.rank());
}

} // namespace numsim::cas
