#ifndef PRINT_MUL_FRACTIONS_H
#define PRINT_MUL_FRACTIONS_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/domain_traits.h>
#include <numsim_cas/core/scalar_number.h>
#include <utility>
#include <vector>

namespace numsim::cas {

template <typename ExprHolder> struct fraction_entry {
  ExprHolder base;
  scalar_number pos_exponent;
};

template <typename Traits, typename HashMap>
requires basic_expression_domain<typename Traits::expression_type>
auto partition_mul_fractions(HashMap const &hash_map)
    -> std::pair<std::vector<typename Traits::expr_holder_t>,
                 std::vector<fraction_entry<typename Traits::expr_holder_t>>> {
  using expr_holder_t = typename Traits::expr_holder_t;
  using pow_type = typename Traits::pow_type;

  std::vector<expr_holder_t> numerator;
  std::vector<fraction_entry<expr_holder_t>> denominator;

  for (auto const &[key, child] : hash_map) {
    if (auto pow_expr = is_same_r<pow_type>(child)) {
      auto const &exponent = pow_expr->get().expr_rhs();
      if (auto num = Traits::try_numeric(exponent);
          num && *num < scalar_number{0}) {
        denominator.push_back({pow_expr->get().expr_lhs(), -(*num)});
        continue;
      }
    }
    numerator.push_back(child);
  }

  return {std::move(numerator), std::move(denominator)};
}

} // namespace numsim::cas

#endif // PRINT_MUL_FRACTIONS_H
