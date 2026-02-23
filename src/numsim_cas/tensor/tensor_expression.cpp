#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/tag_invoke.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor/tensor_negative.h>
#include <numsim_cas/tensor/tensor_zero.h>

namespace numsim::cas {

expression_holder<tensor_expression>
tag_invoke(detail::neg_fn, std::type_identity<tensor_expression>,
           expression_holder<tensor_expression> const &e) {
  if (is_same<tensor_zero>(e))
    return e;

  // -(-x) = x
  if (is_same<tensor_negative>(e)) {
    return e.get<tensor_negative>().expr();
  }

  return make_expression<tensor_negative>(e);
}

} // namespace numsim::cas
