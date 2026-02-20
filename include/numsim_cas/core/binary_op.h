#ifndef BINARY_OP_H
#define BINARY_OP_H

#include <numsim_cas/core/core_fwd.h>
#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/numsim_cas_type_traits.h>
#include <numsim_cas/update_hash.h>
#include <numsim_cas/utility_func.h>

namespace numsim::cas {

/**
 * @class binary_op
 * @brief A binary operation expression class.
 *
 * This class represents a binary operation expression, inheriting from
 * expression_crtp. It provides functionality for handling left-hand side and
 * right-hand side expressions.
 *
 * @tparam ThisBase The base class that implements the binary operation.
 * @tparam BaseLHS The base type of the left-hand side expression.
 * @tparam BaseRHS The base type of the right-hand side expression (defaults to
 * BaseLHS).
 */
template <typename ThisBase, typename BaseLHS = typename ThisBase::expr_t,
          typename BaseRHS = BaseLHS>
class binary_op : public ThisBase {
public:
  /**
   * @brief Type aliases for base types.
   */
  using base = ThisBase;
  using base_expr = ThisBase;

  /**
   * @brief Constructor for binary operation expressions.
   * @tparam ExprLHS Type of the left-hand side expression.
   * @tparam ExprRHS Type of the right-hand side expression.
   * @tparam Args Additional arguments for base class initialization.
   * @param expr_lhs Left-hand side expression.
   * @param expr_rhs Right-hand side expression.
   * @param args Additional arguments for the base class constructor.
   */
  template <typename ExprLHS, typename ExprRHS,
            std::enable_if_t<std::is_base_of_v<BaseLHS, get_type_t<ExprLHS>>,
                             bool> = true,
            std::enable_if_t<std::is_base_of_v<BaseRHS, get_type_t<ExprRHS>>,
                             bool> = true,
            typename... Args>
  binary_op(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs, Args &&...args)
      : base(std::forward<Args>(args)...),
        m_lhs(std::forward<ExprLHS>(expr_lhs)),
        m_rhs(std::forward<ExprRHS>(expr_rhs)) {}

  /**
   * @brief Move constructor.
   * @param data The binary_op object to move from.
   */
  binary_op(binary_op &&data)
      : base(std::forward<base_expr>(data)), m_lhs(std::move(data.m_lhs)),
        m_rhs(std::move(data.m_rhs)) {}

  /**
   * @brief Copy constructor.
   * @param data The binary_op object to copy from.
   */
  binary_op(binary_op const &data)
      : base(static_cast<base_expr const &>(data)), m_lhs(data.m_lhs),
        m_rhs(data.m_rhs) {}

  /**
   * @brief Virtual destructor.
   */
  virtual ~binary_op() {}

  /**
   * @brief Deleted default constructor.
   */
  binary_op() = delete;

  /**
   * @brief Deleted copy assignment operator.
   */
  const binary_op &operator=(binary_op const &) = delete;

  /**
   * @brief Retrieves the left-hand side expression.
   * @return A constant reference to the left-hand side expression.
   */
  [[nodiscard]] inline const auto &expr_lhs() const noexcept { return m_lhs; }

  /**
   * @brief Retrieves the right-hand side expression.
   * @return A constant reference to the right-hand side expression.
   */
  [[nodiscard]] inline const auto &expr_rhs() const noexcept { return m_rhs; }

  /**
   * @brief Retrieves the left-hand side expression.
   * @return A reference to the left-hand side expression.
   */
  [[nodiscard]] inline auto &expr_lhs() noexcept { return m_lhs; }

  /**
   * @brief Retrieves the right-hand side expression.
   * @return A reference to the right-hand side expression.
   */
  [[nodiscard]] inline auto &expr_rhs() noexcept { return m_rhs; }

  template <typename B, typename L, typename R>
  friend bool operator<(binary_op<B, L, R> const &lhs,
                        binary_op<B, L, R> const &rhs);
  template <typename B, typename L, typename R>
  friend bool operator>(binary_op<B, L, R> const &lhs,
                        binary_op<B, L, R> const &rhs);
  template <typename B, typename L, typename R>
  friend bool operator==(binary_op<B, L, R> const &lhs,
                         binary_op<B, L, R> const &rhs);
  template <typename B, typename L, typename R>
  friend bool operator!=(binary_op<B, L, R> const &lhs,
                         binary_op<B, L, R> const &rhs);

protected:
  virtual void update_hash_value() const noexcept override {
    base::m_hash_value =
        update_hash<binary_op<ThisBase, BaseLHS, BaseRHS>>()(*this);
  }
  /**
   * @brief Holds the left-hand side expression.
   */
  expression_holder<BaseLHS> m_lhs;

  /**
   * @brief Holds the right-hand side expression.
   */
  expression_holder<BaseRHS> m_rhs;
};

template <typename BaseT, typename BaseLHS, typename BaseRHS>
bool operator<(binary_op<BaseT, BaseLHS, BaseRHS> const &lhs,
               binary_op<BaseT, BaseLHS, BaseRHS> const &rhs) {
  if (lhs.hash_value() != rhs.hash_value())
    return lhs.hash_value() < rhs.hash_value();
  if (lhs.id() != rhs.id())
    return lhs.id() < rhs.id();
  if (lhs.m_lhs.get().hash_value() != rhs.m_lhs.get().hash_value())
    return lhs.m_lhs.get().hash_value() < rhs.m_lhs.get().hash_value();
  if (lhs.m_rhs.get().hash_value() != rhs.m_rhs.get().hash_value())
    return lhs.m_rhs.get().hash_value() < rhs.m_rhs.get().hash_value();
  if (lhs == rhs)
    return false;
  if (lhs.m_lhs != rhs.m_lhs)
    return lhs.m_lhs < rhs.m_lhs;
  return lhs.m_rhs < rhs.m_rhs;
}

template <typename BaseT, typename BaseLHS, typename BaseRHS>
bool operator>(binary_op<BaseT, BaseLHS, BaseRHS> const &lhs,
               binary_op<BaseT, BaseLHS, BaseRHS> const &rhs) {
  return rhs < lhs;
}

template <typename BaseT, typename BaseLHS, typename BaseRHS>
bool operator==(binary_op<BaseT, BaseLHS, BaseRHS> const &lhs,
                binary_op<BaseT, BaseLHS, BaseRHS> const &rhs) {
  return lhs.m_lhs == rhs.m_lhs && lhs.m_rhs == rhs.m_rhs;
}

template <typename BaseT, typename BaseLHS, typename BaseRHS>
bool operator!=(binary_op<BaseT, BaseLHS, BaseRHS> const &lhs,
                binary_op<BaseT, BaseLHS, BaseRHS> const &rhs) {
  return !(lhs == rhs);
}

template <typename... Args>
struct update_hash<numsim::cas::binary_op<Args...>> {
  std::size_t
  operator()(const numsim::cas::binary_op<Args...> &expr) const noexcept {
    std::size_t seed{0};
    numsim::cas::hash_combine(seed, numsim::cas::binary_op<Args...>::get_id());
    numsim::cas::hash_combine(seed, expr.expr_lhs().get().hash_value());
    numsim::cas::hash_combine(seed, expr.expr_rhs().get().hash_value());
    return seed;
  }
};

} // namespace numsim::cas

#endif // BINARY_OP_H
