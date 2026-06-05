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
   *
   * Copies the numeric assumption manager along with the cached hash. The
   * assumption set is per-node state: when a user writes
   * `auto e2 = make_expression<scalar_constant>(*e1.data())`, the copied
   * node must carry the same assumptions as the source — otherwise the
   * copy loses `positive{}` / `real{}` etc. silently.
   */
  expression(expression const &data)
      : m_assumption(data.m_assumption), m_hash_value(data.m_hash_value) {}

  /**
   * @brief Move constructor.
   * @param data The expression object to move from.
   */
  expression(expression &&data) noexcept
      : m_assumption(std::move(data.m_assumption)),
        m_hash_value(data.m_hash_value) {}

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

  /**
   * @brief Whether this node is a Symbol (named leaf accepting user
   * assertions via `assumption()`).
   *
   * SymPy-style assumption model: only Symbols (named tensor / scalar
   * variables) carry user-asserted facts. Constants (zero, one, identity,
   * literals) and compound expressions return false; their facts are
   * intrinsic to the type or derived from structure + leaves.
   *
   * Default: false. Symbol-class nodes override to return true.
   */
  [[nodiscard]] virtual bool is_symbol() const noexcept { return false; }

  inline auto &assumptions() noexcept { return m_assumption; }

  inline auto const &assumptions() const noexcept { return m_assumption; }

  bool operator==(expression const &rhs) const noexcept;

  bool operator!=(expression const &rhs) const noexcept;

  bool operator<(expression const &rhs) const noexcept;

protected:
  // each concrete node compares against same dynamic type here
  virtual bool equals_same_type(expression const &rhs) const noexcept = 0;
  virtual bool less_than_same_type(expression const &rhs) const noexcept = 0;

  virtual void update_hash_value() const = 0;

  numeric_assumption_manager m_assumption{};
  // NOTE: lazy hash caching is not thread-safe. If multithreading is
  // introduced, protect update_hash_value() with synchronization.
  mutable hash_type m_hash_value{0};
};

} // namespace numsim::cas

#endif // EXPRESSION_H
