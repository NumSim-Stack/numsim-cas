#ifndef SCALAR_ADD_H
#define SCALAR_ADD_H

#include "../n_ary_tree.h"
#include "../scalar/scalar_expression.h"
#include "../symTM_type_traits.h"

namespace numsim::cas {

template <typename ValueType>
class scalar_add final
    : //public expression_crtp<scalar_add<ValueType>, scalar_expression<ValueType>>,
      public n_ary_tree<scalar_expression<ValueType>, scalar_add<ValueType>>
{
public:
  using base = n_ary_tree<scalar_expression<ValueType>, scalar_add<ValueType>>;
  using base_expr = expression_crtp<scalar_add<ValueType>, scalar_expression<ValueType>>;
  using base::base;

  scalar_add():base(){}
  ~scalar_add() = default;
  scalar_add(scalar_add const& add):base(static_cast<base const&>(add)){}
  scalar_add(scalar_add && add):base(std::forward<base>(add)){}

  const scalar_add &operator=(scalar_add &&) = delete;
};

} // namespace numsim::cas

#endif // SCALAR_ADD_H
