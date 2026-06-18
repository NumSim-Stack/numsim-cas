#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/tag_invoke.h>
#include <numsim_cas/scalar/scalar_assume.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/scalar/scalar_globals.h>
#include <numsim_cas/scalar/scalar_negative.h>
#include <numsim_cas/scalar/scalar_positivity_propagation.h>
#include <numsim_cas/scalar/scalar_zero.h>

namespace numsim::cas {

expression_holder<scalar_expression>
tag_invoke(detail::neg_fn, std::type_identity<scalar_expression>,
           expression_holder<scalar_expression> const &e) {
  if (!e.is_valid())
    return get_scalar_zero();

  if (is_same<scalar_zero>(e))
    return get_scalar_zero();

  // -(-x) = x — preserves x's existing assumptions automatically.
  if (is_same<scalar_negative>(e)) {
    return e.get<scalar_negative>().expr();
  }

  // #305 — capture operand sign BEFORE building the negative node so
  // the propagation step can flip pos→neg / nonneg→nonpos / etc.
  const auto operand_view = positivity::read(e);
  auto result = make_expression<scalar_negative>(e);
  positivity::propagate_neg_from_view(operand_view, result);
  return result;
}

} // namespace numsim::cas
