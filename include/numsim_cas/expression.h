#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "assumptions.h"
#include <cstdlib>

namespace numsim::cas {

/**
 * @class expression
 * @brief A base class representing an expression with a hash value.
 *
 * This class provides basic functionality to store and retrieve a hash value,
 * which can be useful for identifying expressions uniquely.
 */
class expression {
public:
  /**
   * @brief Type alias for the hash value.
   */
  using hash_type = std::size_t;

  /**
   * @brief Default constructor.
   */
  expression() = default;

  /**
   * @brief Copy constructor.
   * @param data The expression object to copy from.
   */
  expression(expression const &data) : m_hash_value(data.m_hash_value) {}

  /**
   * @brief Move constructor.
   * @param data The expression object to move from.
   */
  expression(expression &&data) : m_hash_value(data.m_hash_value) {}

  /**
   * @brief Virtual destructor.
   */
  virtual ~expression() = default;

  /**
   * @brief Deleted copy assignment operator.
   *
   * Prevents assignment of expression objects.
   */
  const expression &operator=(expression const &) = delete;

  /**
   * @brief Retrieves the hash value of the expression.
   * @return The hash value.
   */
  hash_type const &hash_value() const {
    if (!m_hash_value) {
      update_hash_value();
    }
    return m_hash_value;
  }

  inline auto &assumptions() noexcept { return m_assumption; }

  inline auto const &assumptions() const noexcept { return m_assumption; }

protected:
  virtual void update_hash_value() const = 0;
  numeric_assumption_manager m_assumption{};
  /**
   * @brief Stores the hash value of the expression.
   */
  mutable hash_type m_hash_value{0};
};

} // namespace numsim::cas

#endif // EXPRESSION_H
