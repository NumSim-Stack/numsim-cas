#ifndef EXPRESSION_CRTP_H
#define EXPRESSION_CRTP_H

#include <cstdlib>
#include "symTM_type_traits.h"

namespace symTM {

/**
 * @class expression_crtp
 * @brief A CRTP (Curiously Recurring Template Pattern) base class for expressions.
 *
 * This class provides a mechanism for static polymorphism using CRTP.
 * It enables derived classes to inherit from a base class while maintaining static type information.
 *
 * @tparam Derived The derived class that inherits from this template.
 * @tparam Base The base class from which Derived inherits.
 */
template <typename Derived, typename Base>
class expression_crtp : public Base {
public:
  /**
   * @brief Type alias for the base class.
   */
  using expr_type = Base;

  /**
   * @brief Default constructor.
   */
  expression_crtp() = default;

  /**
   * @brief Variadic template constructor to forward arguments to the base class constructor.
   * @tparam Args The types of the arguments.
   * @param args The arguments to forward to the base class constructor.
   */
  template <typename... Args>
  expression_crtp(Args &&...args) : Base(std::forward<Args>(args)...) {}

  /**
   * @brief Virtual destructor.
   */
  virtual ~expression_crtp() = default;

  /**
   * @brief Deleted copy assignment operator.
   *
   * Prevents assignment of expression_crtp objects.
   */
  const expression_crtp &operator=(expression_crtp const &) = delete;

  /**
   * @brief Retrieves the unique type identifier of the expression.
   * @return The type identifier.
   */
  [[nodiscard]] static type_id get_id() noexcept {
    return m_id;
  }

private:
  /**
   * @brief Stores the unique type identifier for the derived class.
   */
  static inline auto m_id{detail::expression_id<Derived>::value};
};


} // NAMESPACE symTM

#endif // EXPRESSION_CRTP_H
