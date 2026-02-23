#ifndef SYMBOL_BASE_H
#define SYMBOL_BASE_H

#include <numsim_cas/core/hash_functions.h>

namespace numsim::cas {

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

  ~symbol_base() override {}

  [[nodiscard]] inline auto &name() const noexcept { return m_name; }

  [[nodiscard]] virtual bool is_symbol() const noexcept { return true; }

  template <typename BaseExprT>
  friend bool operator<(symbol_base<BaseExprT> const &lhs,
                        symbol_base<BaseExprT> const &rhs);
  template <typename BaseExprT>
  friend bool operator>(symbol_base<BaseExprT> const &lhs,
                        symbol_base<BaseExprT> const &rhs);
  template <typename BaseExprT>
  friend bool operator==(symbol_base<BaseExprT> const &lhs,
                         symbol_base<BaseExprT> const &rhs);
  template <typename BaseExprT>
  friend bool operator!=(symbol_base<BaseExprT> const &lhs,
                         symbol_base<BaseExprT> const &rhs);

public:
  // assumption_manager<expr_type> m_assumptions;
  // assumption<std::any> m_assumptions;

protected:
  void update_hash_value() const override {
    hash_combine(this->m_hash_value, m_name);
  }

  std::string m_name;
};

template <typename BaseExprT>
bool operator<(symbol_base<BaseExprT> const &lhs,
               symbol_base<BaseExprT> const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

template <typename BaseExprT>
bool operator>(symbol_base<BaseExprT> const &lhs,
               symbol_base<BaseExprT> const &rhs) {
  return rhs < lhs;
}

template <typename BaseExprT>
bool operator==(symbol_base<BaseExprT> const &lhs,
                symbol_base<BaseExprT> const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

template <typename BaseExprT>
bool operator!=(symbol_base<BaseExprT> const &lhs,
                symbol_base<BaseExprT> const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // SYMBOL_BASE_H
