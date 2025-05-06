#ifndef SCALAR_MUL_H
#define SCALAR_MUL_H

#include "symTM/n_ary_tree.h"
#include "symTM/scalar/scalar_expression.h"
#include "symTM/symTM_type_traits.h"

namespace numsim::cas {

template <typename ValueType>
class scalar_mul final
    : public n_ary_tree<scalar_expression<ValueType>, scalar_mul<ValueType>>/*,
      public expression_crtp<scalar_mul<ValueType>, scalar_expression<ValueType>>*/ {
public:
  using base = n_ary_tree<scalar_expression<ValueType>, scalar_mul<ValueType>>;
  //using base_op = n_ary_tree<scalar_expression<ValueType>, scalar_mul<ValueType>>;
  //using base_expr = expression_crtp<scalar_mul<ValueType>, scalar_expression<ValueType>>;

  using base::base;

  scalar_mul():base(){}
  scalar_mul(scalar_mul const &mul):base(static_cast<base const&>(mul)){}
  scalar_mul(scalar_mul && mul):base(std::forward<base>(mul)){}
  ~scalar_mul() = default;

  const scalar_mul &operator=(scalar_mul &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_MUL_H
