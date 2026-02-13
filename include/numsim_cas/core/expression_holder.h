#ifndef EXPRESSION_HOLDER_H
#define EXPRESSION_HOLDER_H

#include <numsim_cas/core/make_negative.h>
#include <numsim_cas/numsim_cas_forward.h>
#include <numsim_cas/numsim_cas_type_traits.h>
#include <type_traits>

namespace numsim::cas {

template <typename ExpressionBase> struct expression_details;

template <typename ExprBase> class expression_holder {
public:
  using expr_type = ExprBase;
  using node_type = ExprBase;
  using node_pointer = typename std::shared_ptr<node_type>;
  using hash_type = typename expr_type::hash_type;

  expression_holder() = default;
  explicit expression_holder(std::shared_ptr<node_type> const &expr)
      : m_expr(expr) {}
  explicit expression_holder(std::shared_ptr<node_type> &&expr)
      : m_expr(std::move(expr)) {}
  expression_holder(expression_holder const &data) : m_expr(data.m_expr) {}
  expression_holder(expression_holder &&data)
      : m_expr(std::move(data.m_expr)) {}
  ~expression_holder() = default;

  expression_holder &operator=(expression_holder const &data) {
    m_expr = data.m_expr;
    return *this;
  }

  expression_holder &operator=(expression_holder &&data) {
    m_expr = std::move(data.m_expr);
    return *this;
  }

  // expression_holder &operator=(std::shared_ptr<node_type> &&data) {
  //   m_expr = std::move(data);
  //   return *this;
  // }

  expression_holder &operator*=(expression_holder &data) {
    if (is_valid()) {
      *this = std::move(*this) * data;
    } else {
      *this = data;
    }
    return *this;
  }

  // template<typename ExprRHS>
  expression_holder &operator*=(expression_holder const &data) {
    if (is_valid()) {
      *this = std::move(*this) * data;
    } else {
      *this = data;
    }
    return *this;
  }

  expression_holder &operator*=(expression_holder &&data) {
    if (is_valid()) {
      *this = std::move(*this) * std::move(data);
    } else {
      *this = std::move(data);
    }
    return *this;
  }

  expression_holder &operator+=(expression_holder &data) {
    if (is_valid()) {
      *this = std::move(*this) + data;
    } else {
      *this = data;
    }
    return *this;
  }

  expression_holder &operator+=(expression_holder const &data) {
    if (is_valid()) {
      *this = std::move(*this) + data;
    } else {
      *this = data;
    }
    return *this;
  }

  expression_holder &operator+=(expression_holder &&data) {
    if (is_valid()) {
      *this = std::move(*this) + std::move(data);
    } else {
      *this = std::move(data);
    }
    return *this;
  }

  constexpr inline auto &data() { return m_expr; }
  constexpr inline const auto &data() const { return m_expr; }
  constexpr inline auto &operator*() { return *m_expr; }
  constexpr inline const auto &operator*() const { return *m_expr; }
  constexpr inline auto *operator->() { return m_expr.get(); }
  constexpr inline const auto *operator->() const { return m_expr.get(); }

  template <typename T = ExprBase> constexpr inline auto &get() {
    if constexpr (std::is_same_v<T, ExprBase>) {
      return *m_expr.get();
    } else {
      return static_cast<T &>(*m_expr.get());
    }
  }

  template <typename T = ExprBase> constexpr inline const auto &get() const {
    if constexpr (std::is_same_v<T, ExprBase>) {
      return *m_expr.get();
    } else {
      return static_cast<const T &>(*m_expr.get());
    }
  }

  constexpr inline bool is_valid() const { return static_cast<bool>(m_expr); }

  constexpr inline expression_holder operator-() { return detail::neg(*this); }

  constexpr inline expression_holder operator-() const {
    return detail::neg(*this);
  }

  constexpr inline auto free() { return m_expr.reset(); }

  template <typename _ExprBase>
  friend std::ostream &operator<<(std::ostream &os,
                                  expression_holder<_ExprBase> const &expr);
  template <typename _ExprBase>
  friend bool operator<(expression_holder<_ExprBase> const &lhs,
                        expression_holder<_ExprBase> const &rhs);
  template <typename _ExprBase>
  friend bool operator>(expression_holder<_ExprBase> const &lhs,
                        expression_holder<_ExprBase> const &rhs);
  template <typename _ExprBase>
  friend bool operator==(expression_holder<_ExprBase> const &lhs,
                         expression_holder<_ExprBase> const &rhs);
  template <typename _ExprBase>
  friend bool operator!=(expression_holder<_ExprBase> const &lhs,
                         expression_holder<_ExprBase> const &rhs);

private:
  std::shared_ptr<node_type> m_expr;
};

template <typename _ExprBase>
bool operator<(expression_holder<_ExprBase> const &lhs,
               expression_holder<_ExprBase> const &rhs) {
  if (lhs.get().hash_value() != rhs.get().hash_value())
    return lhs.get().hash_value() < rhs.get().hash_value();

  if (lhs.get().id() != rhs.get().id())
    return lhs.get().id() < rhs.get().id();

  return false;
}

template <typename _ExprBase>
bool operator>(expression_holder<_ExprBase> const &lhs,
               expression_holder<_ExprBase> const &rhs) {
  return rhs < lhs;
}

template <typename _ExprBase>
bool operator==(expression_holder<_ExprBase> const &lhs,
                expression_holder<_ExprBase> const &rhs) {
  return *lhs == *rhs;
}

template <typename _ExprBase>
bool operator!=(expression_holder<_ExprBase> const &lhs,
                expression_holder<_ExprBase> const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // EXPRESSION_HOLDER_H
