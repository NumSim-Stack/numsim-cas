#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/tag_invoke.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

namespace numsim::cas {

expression_holder<tensor_to_scalar_expression>
tag_invoke(detail::neg_fn, std::type_identity<tensor_to_scalar_expression>,
           expression_holder<tensor_to_scalar_expression> const &e) {
  if (!e.is_valid())
    return make_expression<tensor_to_scalar_zero>();

  if (is_same<tensor_to_scalar_zero>(e))
    return make_expression<tensor_to_scalar_zero>();

  // -(-x) = x
  if (is_same<tensor_to_scalar_negative>(e)) {
    return e.get<tensor_to_scalar_negative>().expr();
  }

  return make_expression<tensor_to_scalar_negative>(e);
}

} // namespace numsim::cas
