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
  expression(expression const &data)
      : m_hash_value(data.m_hash_value),
        m_creation_id(data.m_creation_id) {}

  /**
   * @brief Move constructor.
   * @param data The expression object to move from.
   */
  expression(expression &&data)
      : m_hash_value(data.m_hash_value),
        m_creation_id(data.m_creation_id) {}

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
  hash_type const &hash_value() const;

  [[nodiscard]] virtual type_id id() const noexcept = 0;

  [[nodiscard]] std::size_t creation_id() const noexcept {
    return m_creation_id;
  }

  inline auto &assumptions() noexcept { return m_assumption; }

  inline auto const &assumptions() const noexcept { return m_assumption; }

  bool operator==(expression const &rhs) const noexcept;

  bool operator!=(expression const &rhs) const noexcept;

protected:
  // each concrete node compares against same dynamic type here
  virtual bool equals_same_type(expression const &rhs) const noexcept = 0;

  virtual void update_hash_value() const = 0;

  static inline std::size_t s_next_creation_id{0};

  numeric_assumption_manager m_assumption{};
  // NOTE: lazy hash caching is not thread-safe. If multithreading is
  // introduced, protect update_hash_value() with synchronization.
  mutable hash_type m_hash_value{0};
  std::size_t m_creation_id{s_next_creation_id++};
};

} // namespace numsim::cas

#endif // EXPRESSION_H
