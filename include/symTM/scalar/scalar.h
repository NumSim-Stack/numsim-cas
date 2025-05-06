#ifndef SCALAR_H
#define SCALAR_H

#include "scalar_expression.h"
#include "../symbol_base.h"
#include "../symTM_type_traits.h"

namespace numsim::cas {

// expression
// expression_crtp<scalar_expr>
//

template <typename ValueType>
class scalar final : public symbol_base<expression_crtp<scalar<ValueType>, scalar_expression<ValueType>>>
///*scalar<ValueType>, */scalar_expression<ValueType>>, public
{
public:
  using base_expr = symbol_base<expression_crtp<scalar<ValueType>, scalar_expression<ValueType>>>;

  explicit scalar(std::string const &name)
      : base_expr(name), m_data() {}

  explicit scalar(scalar const &data) = delete;

  explicit scalar(scalar &&data)
      : base_expr(std::move(static_cast<base_expr&&>(data))), m_data() {}

  using base_expr::operator=;
  //using symbol_base<scalar, scalar_expression<ValueType>>::operator-;
  //using symbol_base<scalar, scalar_expression<ValueType>>::operator+=;

  template <typename T, std::enable_if_t<std::is_same_v<ValueType, T>, bool> = true>
  const scalar &operator=(T const &data) {
    m_data = data;
    return *this;
  }

  const scalar &operator=(scalar const &data) {
    this->m_name = data.name();
    m_data = data;
    return *this;
  }

  const scalar &operator=(scalar &&data) {
    this->m_name = data.name();
    m_data = data;
    return *this;
  }

  const auto &data() const { return m_data; }

private:
  ValueType m_data;
};

} // NAMESPACE symTM

#endif // SCALAR_H
