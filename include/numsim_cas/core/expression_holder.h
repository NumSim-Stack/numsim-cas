#ifndef EXPRESSION_HOLDER_H
#define EXPRESSION_HOLDER_H

#include <numsim_cas/core/cas_error.h>
#include <numsim_cas/core/make_negative.h>
#include <numsim_cas/numsim_cas_forward.h>
#include <numsim_cas/numsim_cas_type_traits.h>
#include <cassert>
#include <functional>
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
  expression_holder(expression_holder const &) = default;
  expression_holder(expression_holder &&) = default;
  ~expression_holder() = default;
  expression_holder &operator=(expression_holder const &) = default;
  expression_holder &operator=(expression_holder &&) = default;

  template <typename U>
    requires std::convertible_to<std::decay_t<U>, expression_holder>
  expression_holder &operator*=(U &&data) {
    if (is_valid()) {
      *this = std::move(*this) * std::forward<U>(data);
    } else {
      *this = std::forward<U>(data);
    }
    return *this;
  }

  template <typename U>
    requires std::convertible_to<std::decay_t<U>, expression_holder>
  expression_holder &operator+=(U &&data) {
    if (is_valid()) {
      *this = std::move(*this) + std::forward<U>(data);
    } else {
      *this = std::forward<U>(data);
    }
    return *this;
  }

  constexpr inline auto &data() { return m_expr; }
  constexpr inline const auto &data() const { return m_expr; }

  inline auto &operator*() {
    throw_if_invalid();
    return *m_expr;
  }
  inline const auto &operator*() const {
    throw_if_invalid();
    return *m_expr;
  }
  inline auto *operator->() {
    throw_if_invalid();
    return m_expr.get();
  }
  inline const auto *operator->() const {
    throw_if_invalid();
    return m_expr.get();
  }

  template <typename T = ExprBase> inline auto &get() {
    throw_if_invalid();
    if constexpr (std::is_same_v<T, ExprBase>) {
      return *m_expr.get();
    } else {
      assert(dynamic_cast<T *>(m_expr.get()) != nullptr);
      return static_cast<T &>(*m_expr.get());
    }
  }

  template <typename T = ExprBase> inline const auto &get() const {
    throw_if_invalid();
    if constexpr (std::is_same_v<T, ExprBase>) {
      return *m_expr.get();
    } else {
      assert(dynamic_cast<const T *>(m_expr.get()) != nullptr);
      return static_cast<const T &>(*m_expr.get());
    }
  }

  constexpr inline bool is_valid() const { return static_cast<bool>(m_expr); }

  constexpr inline expression_holder operator-() { return detail::neg(*this); }

  constexpr inline expression_holder operator-() const {
    return detail::neg(*this);
  }

  constexpr inline auto free() { return m_expr.reset(); }

  template <typename ExprBaseT>
  friend std::ostream &operator<<(std::ostream &os,
                                  expression_holder<ExprBaseT> const &expr);
  template <typename ExprBaseT>
  friend bool operator<(expression_holder<ExprBaseT> const &lhs,
                        expression_holder<ExprBaseT> const &rhs);
  template <typename ExprBaseT>
  friend bool operator>(expression_holder<ExprBaseT> const &lhs,
                        expression_holder<ExprBaseT> const &rhs);
  template <typename ExprBaseT>
  friend bool operator==(expression_holder<ExprBaseT> const &lhs,
                         expression_holder<ExprBaseT> const &rhs);
  template <typename ExprBaseT>
  friend bool operator!=(expression_holder<ExprBaseT> const &lhs,
                         expression_holder<ExprBaseT> const &rhs);

private:
  inline void throw_if_invalid() const {
    if (!m_expr) {
      throw invalid_expression_error(
          "expression_holder: access to invalid (null) expression");
    }
  }

  std::shared_ptr<node_type> m_expr;
};

template <typename ExprBaseT>
bool operator<(expression_holder<ExprBaseT> const &lhs,
               expression_holder<ExprBaseT> const &rhs) {
  if (!lhs.is_valid() || !rhs.is_valid()) {
    throw invalid_expression_error(
        "expression_holder::operator<: comparing invalid (null) expression");
  }
  return lhs.get() < rhs.get();
}

template <typename ExprBaseT>
bool operator>(expression_holder<ExprBaseT> const &lhs,
               expression_holder<ExprBaseT> const &rhs) {
  return rhs < lhs;
}

template <typename ExprBaseT>
bool operator==(expression_holder<ExprBaseT> const &lhs,
                expression_holder<ExprBaseT> const &rhs) {
  if (!lhs.is_valid() || !rhs.is_valid()) {
    throw invalid_expression_error(
        "expression_holder::operator==: comparing invalid (null) expression");
  }
  return *lhs == *rhs;
}

template <typename ExprBaseT>
bool operator!=(expression_holder<ExprBaseT> const &lhs,
                expression_holder<ExprBaseT> const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // EXPRESSION_HOLDER_H
