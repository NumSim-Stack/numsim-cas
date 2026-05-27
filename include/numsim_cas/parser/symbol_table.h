#ifndef NUMSIM_CAS_PARSER_SYMBOL_TABLE_H
#define NUMSIM_CAS_PARSER_SYMBOL_TABLE_H

#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/tensor/tensor_expression.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>

namespace numsim::cas::parser {

/**
 * @class symbol_table
 * @brief Maps identifier names to scalar / tensor variable holders,
 *        populated on the fly as the parser encounters identifiers.
 *
 * Lookup rules:
 *  - **Scalars** are declared implicitly on first reference. Calling
 *    `get_or_declare_scalar("x")` creates the holder if absent,
 *    returns the same holder on every subsequent call.
 *  - **Tensors** must be declared explicitly via
 *    `get_or_declare_tensor("A", rank=2, dim=3)`. Subsequent calls
 *    with the same (rank, dim) return the same holder. Mismatched
 *    (rank, dim) on re-declaration raise `redeclaration_error`.
 *
 * Cross-type rules:
 *  - Using an identifier as a scalar after it was declared as a tensor
 *    (or vice versa) raises `type_collision_error`.
 *
 * Errors raised from `symbol_table` carry no source position — they
 * are constructed with an empty source view. When the parser is the
 * caller, Phase-2 actions catch and re-throw with full position
 * context. Direct user calls receive position-less errors with a
 * descriptive message.
 *
 * **Thread-safety**: instances are not thread-safe. Multiple parses
 * sharing one table is undefined behaviour. Use one table per parse,
 * or guard with external synchronisation.
 *
 * **Persistence on parse failure**: declarations are committed as
 * they happen. If parsing throws partway through, declarations made
 * up to that point remain in the table. Callers that want
 * all-or-nothing semantics should discard the table and start over
 * on failure.
 */
class symbol_table {
public:
  symbol_table() = default;
  symbol_table(symbol_table const &) = default;
  symbol_table(symbol_table &&) noexcept = default;
  symbol_table &operator=(symbol_table const &) = default;
  symbol_table &operator=(symbol_table &&) noexcept = default;
  ~symbol_table() = default;

  /**
   * @brief Look up an existing scalar declaration or declare a new one.
   *
   * @throws type_collision_error if `name` was previously declared
   *         as a tensor.
   */
  expression_holder<scalar_expression>
  get_or_declare_scalar(std::string_view name);

  /**
   * @brief Look up an existing tensor declaration or declare a new one.
   *
   * On re-declaration the (rank, dim) must match the prior values.
   *
   * @throws redeclaration_error if `name` was previously declared as
   *         a tensor with different (rank, dim).
   * @throws type_collision_error if `name` was previously declared
   *         as a scalar.
   */
  expression_holder<tensor_expression>
  get_or_declare_tensor(std::string_view name, std::size_t rank,
                        std::size_t dim);

  /// True if any declaration exists for the given name.
  [[nodiscard]] bool has(std::string_view name) const noexcept;

  /// If `name` is a tensor, returns `{rank, dim}`; otherwise `nullopt`.
  /// `nullopt` is also returned for an unknown name or a scalar.
  [[nodiscard]] std::optional<std::pair<std::size_t, std::size_t>>
  tensor_shape(std::string_view name) const;

  /// Number of declarations currently in the table.
  [[nodiscard]] std::size_t size() const noexcept { return m_entries.size(); }

private:
  struct scalar_entry {
    expression_holder<scalar_expression> expr;
  };
  struct tensor_entry {
    expression_holder<tensor_expression> expr;
    std::size_t rank;
    std::size_t dim;
  };
  using entry = std::variant<scalar_entry, tensor_entry>;

  std::unordered_map<std::string, entry> m_entries;
};

} // namespace numsim::cas::parser

#endif // NUMSIM_CAS_PARSER_SYMBOL_TABLE_H
