#ifndef UNARY_OP_H
#define UNARY_OP_H

#include "expression_crtp.h"
#include "numsim_cas_type_traits.h"
#include "utility_func.h"

namespace numsim::cas {

template <typename Derived, typename Base, typename ExprType = Base>
class unary_op
    : public expression_crtp<unary_op<Derived, Base, ExprType>, Base> {
public:
  using base = expression_crtp<unary_op<Derived, Base, ExprType>, Base>;
  using base_expr = Base;

  template <
      typename Expr,
      std::enable_if_t<
          std::is_base_of_v<ExprType, std::remove_pointer_t<get_type_t<Expr>>>,
          bool> = true,
      typename... Args>
  explicit unary_op(Expr &&expr, Args &&...args)
      : base(std::forward<Args>(args)...), m_expr(std::forward<Expr>(expr)) {
    update_hash_value();
  }

  explicit unary_op(unary_op &&data)
      : base(std::move(static_cast<base_expr &&>(data))),
        m_expr(std::move(data.m_expr)) {}

  explicit unary_op(unary_op const &data)
      : base(static_cast<const base_expr &>(data)), m_expr(data.m_expr) {}

  virtual ~unary_op() = default;
  const unary_op &operator=(unary_op const &) = delete;
  [[nodiscard]] constexpr inline auto &expr() noexcept { return m_expr; }
  [[nodiscard]] constexpr inline const auto &expr() const noexcept {
    return m_expr;
  }

  template <typename _Derived, typename _Base, typename _ExprType>
  friend bool operator<(unary_op<_Derived, _Base, _ExprType> const &lhs,
                        unary_op<_Derived, _Base, _ExprType> const &rhs);
  template <typename _Derived, typename _Base, typename _ExprType>
  friend bool operator>(unary_op<_Derived, _Base, _ExprType> const &lhs,
                        unary_op<_Derived, _Base, _ExprType> const &rhs);
  template <typename _Derived, typename _Base, typename _ExprType>
  friend bool operator==(unary_op<_Derived, _Base, _ExprType> const &lhs,
                         unary_op<_Derived, _Base, _ExprType> const &rhs);
  template <typename _Derived, typename _Base, typename _ExprType>
  friend bool operator!=(unary_op<_Derived, _Base, _ExprType> const &lhs,
                         unary_op<_Derived, _Base, _ExprType> const &rhs);

protected:
  void update_hash_value() {
    hash_combine(base::m_hash_value, base::get_id());
    hash_combine(base::m_hash_value, m_expr.get().hash_value());
  }

  expression_holder<ExprType> m_expr;
};

template <typename _Derived, typename _Base, typename _ExprType>
bool operator<(unary_op<_Derived, _Base, _ExprType> const &lhs,
               unary_op<_Derived, _Base, _ExprType> const &rhs) {
  return lhs.expr() < rhs.expr();
}

template <typename _Derived, typename _Base, typename _ExprType>
bool operator>(unary_op<_Derived, _Base, _ExprType> const &lhs,
               unary_op<_Derived, _Base, _ExprType> const &rhs) {
  return !(lhs < rhs);
}

template <typename _Derived, typename _Base, typename _ExprType>
bool operator==(unary_op<_Derived, _Base, _ExprType> const &lhs,
                unary_op<_Derived, _Base, _ExprType> const &rhs) {
  return lhs.expr() == rhs.expr();
}

template <typename _Derived, typename _Base, typename _ExprType>
bool operator!=(unary_op<_Derived, _Base, _ExprType> const &lhs,
                unary_op<_Derived, _Base, _ExprType> const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas

#endif // UNARY_OP_H
