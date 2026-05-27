#ifndef NUMSIM_CAS_PARSER_PARSER_H
#define NUMSIM_CAS_PARSER_PARSER_H

#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/parser/symbol_table.h>
#include <numsim_cas/scalar/scalar_expression.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

#include <string_view>
#include <variant>

namespace numsim::cas::parser {

/**
 * @brief Result of `parse()` — a discriminated union over the three
 *        expression domains.
 *
 * The variant alternative tells the caller which domain the top-level
 * expression landed in. Use `std::visit` or `std::holds_alternative`
 * to dispatch, or one of the `parse_scalar` / `parse_tensor` /
 * `parse_t2s` convenience wrappers if you already know which domain
 * to expect.
 */
using parsed_expression =
    std::variant<expression_holder<scalar_expression>,
                 expression_holder<tensor_expression>,
                 expression_holder<tensor_to_scalar_expression>>;

/**
 * @brief Parse `source` into an expression, populating `syms` with any
 *        identifier declarations encountered during the parse.
 *
 * Identifiers are typed lazily:
 *   - Bare identifiers (`x`, `velocity`) are scalar variables, declared
 *     implicitly on first use.
 *   - `Name{rank=R, dim=D}` is a tensor declaration. Subsequent bare
 *     references to `Name` resolve to the same tensor.
 *
 * Re-declarations must match; cross-type collisions are rejected. See
 * `symbol_table` for the full rules.
 *
 * @throws parse_error (or any subclass) on lexical / syntactic /
 *         semantic failure. The exception's `what()` includes a
 *         snippet-with-caret rendering of the offending source line.
 *
 * **Symbol-table semantics on failure**: declarations are
 * non-transactional — anything declared before the failure stays in
 * `syms`. Caller can discard the table for all-or-nothing semantics.
 *
 * **Thread-safety**: not reentrant on the same `symbol_table`. Use
 * one table per concurrent parse, or guard with external
 * synchronisation.
 */
[[nodiscard]] parsed_expression parse(std::string_view source,
                                      symbol_table &syms);

/**
 * @brief Parse and require a scalar result.
 *
 * @throws type_mismatch_error if the parsed expression is not in the
 *         scalar domain.
 * @throws parse_error (or subclass) on parse failure.
 */
[[nodiscard]] expression_holder<scalar_expression>
parse_scalar(std::string_view source, symbol_table &syms);

/**
 * @brief Parse and require a tensor result.
 *
 * @throws type_mismatch_error if the parsed expression is not in the
 *         tensor domain.
 */
[[nodiscard]] expression_holder<tensor_expression>
parse_tensor(std::string_view source, symbol_table &syms);

/**
 * @brief Parse and require a tensor-to-scalar result.
 *
 * @throws type_mismatch_error if the parsed expression is not in the
 *         tensor-to-scalar domain.
 */
[[nodiscard]] expression_holder<tensor_to_scalar_expression>
parse_t2s(std::string_view source, symbol_table &syms);

} // namespace numsim::cas::parser

#endif // NUMSIM_CAS_PARSER_PARSER_H
