#ifndef BINARY_OP_H
#define BINARY_OP_H

#include "expression_crtp.h"
#include "expression_holder.h"
#include "symTM_type_traits.h"
#include "utility_func.h"
#include "symTM_type_traits.h"
#include "utility_func.h"

namespace numsim::cas {

/**
 * @class binary_op
 * @brief A binary operation expression class.
 *
 * This class represents a binary operation expression, inheriting from expression_crtp.
 * It provides functionality for handling left-hand side and right-hand side expressions.
 *
 * @tparam Derived The derived class that implements the binary operation.
 * @tparam BaseLHS The base type of the left-hand side expression.
 * @tparam BaseRHS The base type of the right-hand side expression (defaults to BaseLHS).
 */
template <typename Derived, typename BaseLHS, typename BaseRHS = BaseLHS>
class binary_op : public expression_crtp<binary_op<Derived, BaseLHS, BaseRHS>,
                                         base_expression_t<BaseLHS, BaseRHS>> {
public:
  /**
   * @brief Type aliases for base types.
   */
  using base = expression_crtp<binary_op<Derived, BaseLHS, BaseRHS>,
                               base_expression_t<BaseLHS, BaseRHS>>;
  using base_expr = base_expression_t<BaseLHS, BaseRHS>;

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
            std::enable_if_t<std::is_base_of_v<BaseLHS, get_type_t<ExprLHS>>, bool> = true,
            std::enable_if_t<std::is_base_of_v<BaseRHS, get_type_t<ExprRHS>>, bool> = true,
            typename... Args>
  binary_op(ExprLHS &&expr_lhs, ExprRHS &&expr_rhs, Args &&...args)
      : base(std::forward<Args>(args)...),
        m_lhs(std::forward<ExprLHS>(expr_lhs)),
        m_rhs(std::forward<ExprRHS>(expr_rhs)) {
    update_hash_imp();
  }

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



protected:
  /**
   * @brief Holds the left-hand side expression.
   */
  expression_holder<BaseLHS> m_lhs;

  /**
   * @brief Holds the right-hand side expression.
   */
  expression_holder<BaseRHS> m_rhs;


private:
  /**
   * @brief Updates the hash value of the binary operation.
   */
  inline void update_hash_imp(){
    if constexpr (is_detected_v<has_update_hash, Derived>){
      static_cast<Derived&>(*this).update_hash();
    }else{
      static const auto id{base::get_id()};
      base::m_hash_value = 0;
      hash_combine(base::m_hash_value, id);
      hash_combine(base::m_hash_value, m_lhs.get().hash_value());
      hash_combine(base::m_hash_value, m_rhs.get().hash_value());
    }
  }
};

} // NAMESPACE symTM

#endif // BINARY_OP_H
