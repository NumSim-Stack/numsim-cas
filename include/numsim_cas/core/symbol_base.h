#ifndef SYMBOL_BASE_H
#define SYMBOL_BASE_H

#include <numsim_cas/core/hash_functions.h>

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

template <typename BaseExpr> class symbol_base : public BaseExpr {
public:
  using expr_t = typename BaseExpr::expr_t;
  using base_t = BaseExpr;
  // using epxr_type_traits = expression_type_traits<expr_type>;

  symbol_base() = delete;
  symbol_base(symbol_base const &) noexcept = delete;
  symbol_base(symbol_base &&data) noexcept
      : base_t(std::move(static_cast<base_t &&>(data))),
        m_name(std::move(data.m_name)) {}

  template <typename... Args>
  explicit symbol_base(std::string const &name, Args &&...args) noexcept
      : base_t(std::forward<Args>(args)...), m_name(name) {}

  virtual ~symbol_base() {}

  [[nodiscard]] inline auto &name() const noexcept { return m_name; }

  [[nodiscard]] virtual bool is_symbol() const noexcept { return true; }

  template <typename _BaseExpr>
  friend bool operator<(symbol_base<_BaseExpr> const &lhs,
                        symbol_base<_BaseExpr> const &rhs);
  template <typename _BaseExpr>
  friend bool operator>(symbol_base<_BaseExpr> const &lhs,
                        symbol_base<_BaseExpr> const &rhs);
  template <typename _BaseExpr>
  friend bool operator==(symbol_base<_BaseExpr> const &lhs,
                         symbol_base<_BaseExpr> const &rhs);
  template <typename _BaseExpr>
  friend bool operator!=(symbol_base<_BaseExpr> const &lhs,
                         symbol_base<_BaseExpr> const &rhs);

public:
  // assumption_manager<expr_type> m_assumptions;
  // assumption<std::any> m_assumptions;

protected:
  virtual void update_hash_value() const override {
    hash_combine(this->m_hash_value, m_name);
  }

  std::string m_name;
};

template <typename _BaseExpr>
bool operator<(symbol_base<_BaseExpr> const &lhs,
               symbol_base<_BaseExpr> const &rhs) {
  std::cout << lhs.m_name << " " << rhs.m_name << std::endl;
  std::cout << lhs.hash_value() << " " << rhs.hash_value() << std::endl;
  return lhs.hash_value() < rhs.hash_value();
}

template <typename _BaseExpr>
bool operator>(symbol_base<_BaseExpr> const &lhs,
               symbol_base<_BaseExpr> const &rhs) {
  return rhs < lhs;
}

template <typename _BaseExpr>
bool operator==(symbol_base<_BaseExpr> const &lhs,
                symbol_base<_BaseExpr> const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

template <typename _BaseExpr>
bool operator!=(symbol_base<_BaseExpr> const &lhs,
                symbol_base<_BaseExpr> const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // SYMBOL_BASE_H
