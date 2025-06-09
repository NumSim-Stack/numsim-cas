#ifndef SCALAR_SUB_H
#define SCALAR_SUB_H

#include "../n_ary_tree.h"
#include "scalar_expression.h"
#include "../numsim_cas_type_traits.h"

namespace numsim::cas {

//template <typename ValueType>
//class scalar_sub final
//    : public n_ary_tree<scalar_expression<ValueType>, scalar_sub<ValueType>>
//{
//public:
//  using base = n_ary_tree<scalar_expression<ValueType>, scalar_sub<ValueType>>;
//  using base::base;
//  scalar_sub():base(){}
//  ~scalar_sub() = default;
//  scalar_sub(scalar_sub const& add):base(static_cast<base const&>(add)){}
//  scalar_sub(scalar_sub && add):base(std::forward<base>(add)){}
//  const scalar_sub &operator=(scalar_sub &&) = delete;
//};

} // namespace numsim::cas


#endif // SCALAR_SUB_H
