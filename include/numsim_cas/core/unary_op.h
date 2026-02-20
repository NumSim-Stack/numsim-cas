#ifndef UNARY_OP_H
#define UNARY_OP_H

#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/core/hash_functions.h>

#include <type_traits>
#include <utility>

namespace numsim::cas {

/**
 * @tparam Derived   The concrete node type (e.g. scalar_negative)
 * @tparam ThisBase  The node base you inherit from (typically
 * visitable_impl<ExprBase, Derived, ...>)
 * @tparam ExprBase  The expression base stored in the child holder (defaults to
 * Base::expr_type if available)
 */
template <typename ThisBase, typename ExprBase = typename ThisBase::expr_t>
class unary_op : public ThisBase {
public:
  using base_t = ThisBase;
  using expr_base_t = ExprBase;
  using expr_holder_t = expression_holder<expr_base_t>;

  // Construct from anything that can construct expr_holder_t, plus any extra
  // args for Base
  template <typename Expr, typename... Args,
            std::enable_if_t<std::is_constructible_v<expr_holder_t, Expr &&>,
                             int> = 0>
  explicit unary_op(Expr &&expr, Args &&...args)
      : base_t(std::forward<Args>(args)...), m_expr(std::forward<Expr>(expr)) {}

  unary_op() = default;
  unary_op(unary_op const &) = default;
  unary_op(unary_op &&) noexcept = default;

  unary_op &operator=(unary_op const &) = delete;
  unary_op &operator=(unary_op &&) noexcept = default;

  /**
   * @brief Virtual destructor.
   */
  virtual ~unary_op() = default;

  [[nodiscard]] constexpr expr_holder_t &expr() noexcept { return m_expr; }
  [[nodiscard]] constexpr expr_holder_t const &expr() const noexcept {
    return m_expr;
  }

protected:
  virtual void update_hash_value() const noexcept override {
    this->m_hash_value = 0;
    hash_combine(this->m_hash_value, this->get_id());
    if (m_expr.is_valid()) {
      hash_combine(this->m_hash_value, m_expr.get().hash_value());
    }
  }

  expr_holder_t m_expr;
};

template <typename BaseLHS, typename ExprBaseLHS, typename BaseRHS,
          typename ExprBaseRHS>
bool operator<(unary_op<BaseLHS, ExprBaseLHS> const &lhs,
               unary_op<BaseRHS, ExprBaseRHS> const &rhs) {
  if (lhs.hash_value() != rhs.hash_value())
    return lhs.hash_value() < rhs.hash_value();
  if (lhs.id() != rhs.id())
    return lhs.id() < rhs.id();
  if (lhs.expr().get().hash_value() != rhs.expr().get().hash_value())
    return lhs.expr().get().hash_value() < rhs.expr().get().hash_value();
  return lhs.expr() < rhs.expr();
}

template <typename BaseLHS, typename ExprBaseLHS, typename BaseRHS,
          typename ExprBaseRHS>
bool operator>(unary_op<BaseLHS, ExprBaseLHS> const &lhs,
               unary_op<BaseRHS, ExprBaseRHS> const &rhs) {
  return rhs < lhs;
}

template <typename BaseLHS, typename ExprBaseLHS, typename BaseRHS,
          typename ExprBaseRHS>
bool operator==(unary_op<BaseLHS, ExprBaseLHS> const &lhs,
                unary_op<BaseRHS, ExprBaseRHS> const &rhs) {
  if (lhs.id() != rhs.id())
    return false;
  return lhs.expr() == rhs.expr();
}

template <typename BaseLHS, typename ExprBaseLHS, typename BaseRHS,
          typename ExprBaseRHS>
bool operator!=(unary_op<BaseLHS, ExprBaseLHS> const &lhs,
                unary_op<BaseRHS, ExprBaseRHS> const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // UNARY_OP_H
