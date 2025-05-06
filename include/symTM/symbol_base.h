#ifndef SYMBOL_BASE_H
#define SYMBOL_BASE_H

#include <string>
#include "symTM_type_traits.h"
#include "assumptions.h"
#include "utility_func.h"


namespace numsim::cas {

//template<typename ExpressionType>
//struct expression_type_traits;
//template<typename T>
//struct expression_type_traits<scalar_expression<T>>
//{
//  using symbol_type   = scalar<T>;
//  using add_type      = scalar_add<T>;
//  using negative_type = scalar_negative<T>;
//};



template<typename Base>
class symbol_base : public Base
{
public:
  using expr_type = typename Base::expr_type;
  //using epxr_type_traits = expression_type_traits<expr_type>;

  symbol_base() = delete;
  symbol_base(symbol_base const &)noexcept = delete;
  symbol_base(symbol_base && data)noexcept:Base(std::move(static_cast<Base&&>(data))),m_name(std::move(data.m_name))
  {}

  template<typename ...Args>
  explicit symbol_base(std::string const &name, Args&&...args) noexcept : Base(std::forward<Args>(args)...),m_name(name) {
    hash_combine(this->m_hash_value, m_name);
  }

  virtual ~symbol_base() {}

  [[nodiscard]] inline auto& name()const noexcept{
    return m_name;
  }

  [[nodiscard]] virtual bool is_symbol() const noexcept {return true;}

public:
  assumption_manager<expr_type> m_assumptions;

protected:
  std::string m_name;
};

} // NAMESPACE symTM

#endif // SYMBOL_BASE_H
