#ifndef SYMBOL_BASE_H
#define SYMBOL_BASE_H

#include <numsim_cas/core/hash_functions.h>

#include <utility>

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
  explicit symbol_base(std::string const &name, Args &&...args)
      : base_t(std::forward<Args>(args)...), m_name(name) {}

  ~symbol_base() override {}

  [[nodiscard]] inline auto &name() const noexcept { return m_name; }

  [[nodiscard]] bool is_symbol() const noexcept override { return true; }

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
    this->m_hash_value = 0;
    hash_combine(this->m_hash_value, m_name);
  }

  std::string m_name;
};

namespace detail {

// Shape belongs to a tensor symbol's identity: the same name at another dim
// or rank denotes a different tensor. Scalar symbols carry no shape.
template <typename Symbol>
[[nodiscard]] inline std::pair<std::size_t, std::size_t>
symbol_shape(Symbol const &symbol) {
  if constexpr (requires {
                  symbol.dim();
                  symbol.rank();
                }) {
    return {symbol.dim(), symbol.rank()};
  } else {
    return {0, 0};
  }
}

} // namespace detail

template <typename BaseExprT>
bool operator<(symbol_base<BaseExprT> const &lhs,
               symbol_base<BaseExprT> const &rhs) {
  if (lhs.hash_value() != rhs.hash_value())
    return lhs.hash_value() < rhs.hash_value();
  // The hash covers only the name, so colliding names need a real tiebreak.
  if (lhs.name() != rhs.name())
    return lhs.name() < rhs.name();
  return detail::symbol_shape(lhs) < detail::symbol_shape(rhs);
}

template <typename BaseExprT>
bool operator>(symbol_base<BaseExprT> const &lhs,
               symbol_base<BaseExprT> const &rhs) {
  return rhs < lhs;
}

template <typename BaseExprT>
bool operator==(symbol_base<BaseExprT> const &lhs,
                symbol_base<BaseExprT> const &rhs) {
  return lhs.hash_value() == rhs.hash_value() && lhs.name() == rhs.name() &&
         detail::symbol_shape(lhs) == detail::symbol_shape(rhs);
}

template <typename BaseExprT>
bool operator!=(symbol_base<BaseExprT> const &lhs,
                symbol_base<BaseExprT> const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // SYMBOL_BASE_H
