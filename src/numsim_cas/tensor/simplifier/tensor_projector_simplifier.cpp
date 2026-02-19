#include <numsim_cas/tensor/simplifier/tensor_projector_simplifier.h>

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/tensor/projector_algebra.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_functions.h>
#include <numsim_cas/tensor/tensor_operators.h>
#include <ranges>

namespace numsim::cas {

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
          auto rule = contraction_rule(k_lhs, k_rhs);
          if (rule) {
            auto dim = proj_lhs.dim();
            switch (*rule) {
            case ContractionRule::Idempotent:
              m_result = apply_projection(k_lhs, inner_info->argument);
              return;
            case ContractionRule::Zero:
              m_result = make_expression<tensor_zero>(dim, v.rank());
              return;
            case ContractionRule::LhsSubspace:
              m_result = apply_projection(k_lhs, inner_info->argument);
              return;
            case ContractionRule::RhsSubspace:
              m_result = apply_projection(k_rhs, inner_info->argument);
              return;
            }
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
            auto rule = contraction_rule(k_lhs, k_rhs);
            if (rule) {
              switch (*rule) {
              case ContractionRule::Idempotent:
                m_result = lhs;
                return;
              case ContractionRule::Zero:
                m_result =
                    make_expression<tensor_zero>(proj_lhs.dim(), v.rank());
                return;
              case ContractionRule::LhsSubspace:
                m_result = lhs;
                return;
              case ContractionRule::RhsSubspace:
                m_result = rhs;
                return;
              }
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
          auto new_expr = apply_projection(
              *combined, entries[i].argument);
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
