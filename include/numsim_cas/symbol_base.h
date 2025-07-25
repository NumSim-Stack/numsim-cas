#ifndef SYMBOL_BASE_H
#define SYMBOL_BASE_H

#include "assumptions.h"
#include "numsim_cas_type_traits.h"
#include "utility_func.h"
#include <string>

namespace numsim::cas {

// template<typename ExpressionType>
// struct expression_type_traits;
// template<typename T>
// struct expression_type_traits<scalar_expression<T>>
//{
//   using symbol_type   = scalar<T>;
//   using add_type      = scalar_add<T>;
//   using negative_type = scalar_negative<T>;
// };

template <typename BaseExpr, typename Derived>
class symbol_base : public expression_crtp<Derived, BaseExpr> {
public:
  using expr_type = BaseExpr;
  using base_t = expression_crtp<Derived, BaseExpr>;
  // using epxr_type_traits = expression_type_traits<expr_type>;

  symbol_base() = delete;
  symbol_base(symbol_base const &) noexcept = delete;
  symbol_base(symbol_base &&data) noexcept
      : base_t(std::move(static_cast<base_t &&>(data))),
        m_name(std::move(data.m_name)) {}

  template <typename... Args>
  explicit symbol_base(std::string const &name, Args &&...args) noexcept
      : base_t(std::forward<Args>(args)...), m_name(name) {
    update_hash_value();
  }

  virtual ~symbol_base() {}

  [[nodiscard]] inline auto &name() const noexcept { return m_name; }

  [[nodiscard]] virtual bool is_symbol() const noexcept { return true; }

  template <typename _BaseExpr, typename _Derived>
  friend bool operator<(symbol_base<_BaseExpr, _Derived> const &lhs,
                        symbol_base<_BaseExpr, _Derived> const &rhs);
  template <typename _BaseExpr, typename _Derived>
  friend bool operator>(symbol_base<_BaseExpr, _Derived> const &lhs,
                        symbol_base<_BaseExpr, _Derived> const &rhs);
  template <typename _BaseExpr, typename _Derived>
  friend bool operator==(symbol_base<_BaseExpr, _Derived> const &lhs,
                         symbol_base<_BaseExpr, _Derived> const &rhs);
  template <typename _BaseExpr, typename _Derived>
  friend bool operator!=(symbol_base<_BaseExpr, _Derived> const &lhs,
                         symbol_base<_BaseExpr, _Derived> const &rhs);

public:
  assumption_manager<expr_type> m_assumptions;

protected:
  void update_hash_value() { hash_combine(this->m_hash_value, m_name); }
  std::string m_name;
};

template <typename _BaseExpr, typename _Derived>
bool operator<(symbol_base<_BaseExpr, _Derived> const &lhs,
               symbol_base<_BaseExpr, _Derived> const &rhs) {
  return lhs.m_hash_value < rhs.m_hash_value;
}

template <typename _BaseExpr, typename _Derived>
bool operator>(symbol_base<_BaseExpr, _Derived> const &lhs,
               symbol_base<_BaseExpr, _Derived> const &rhs) {
  return !(lhs < rhs);
}

template <typename _BaseExpr, typename _Derived>
bool operator==(symbol_base<_BaseExpr, _Derived> const &lhs,
                symbol_base<_BaseExpr, _Derived> const &rhs) {
  return lhs.m_hash_value == rhs.m_hash_value;
}

template <typename _BaseExpr, typename _Derived>
bool operator!=(symbol_base<_BaseExpr, _Derived> const &lhs,
                symbol_base<_BaseExpr, _Derived> const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // SYMBOL_BASE_H
