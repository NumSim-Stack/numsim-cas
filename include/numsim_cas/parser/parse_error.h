#ifndef NUMSIM_CAS_PARSER_PARSE_ERROR_H
#define NUMSIM_CAS_PARSER_PARSE_ERROR_H

#include <numsim_cas/core/cas_error.h>

#include <cstddef>
#include <string>
#include <string_view>

namespace numsim::cas::parser {

/**
 * @class parse_error
 * @brief Base class for all errors raised by the string parser.
 *
 * Carries a byte offset into the source, a 1-based line and column, and
 * (when a source string is supplied) renders a snippet-with-caret in
 * `what()` for direct presentation to the user:
 *
 * ```text
 * parse error at 1:11: expected ')' to close call to 'sin'
 *   sin(x + 2 + y
 *             ^
 * ```
 *
 * When constructed without a source (or with an empty source view) the
 * formatted message degrades to just the body — this is the path used
 * by `symbol_table` which has no source context. Phase-2 parser
 * actions will catch and re-throw position-less errors with full
 * context.
 *
 * All subclasses inherit from `cas_error` so calling code can use a
 * single `catch (cas_error const&)` to handle parsing and evaluation
 * failures uniformly.
 */
class parse_error : public cas_error {
public:
  /// Construct with optional source context. Pass an empty `source` and
  /// `byte_offset = 0` for errors raised outside the parser.
  parse_error(std::string message, std::size_t byte_offset,
              std::string_view source);

  /// Byte offset into the source where the error was detected, clamped
  /// to `source.size()`. Returns 0 if no source was supplied.
  [[nodiscard]] std::size_t position() const noexcept { return m_pos; }

  /// 1-based line number. Returns 1 if no source was supplied.
  [[nodiscard]] std::size_t line() const noexcept { return m_line; }

  /// 1-based column number within the line. Returns 1 if no source
  /// was supplied.
  [[nodiscard]] std::size_t column() const noexcept { return m_col; }

  /// True if a non-empty source view was supplied at construction.
  /// When false, `what()` contains just the body — no line/column or
  /// snippet rendering.
  [[nodiscard]] bool has_position() const noexcept { return m_has_position; }

protected:
  std::size_t m_pos = 0;
  std::size_t m_line = 1;
  std::size_t m_col = 1;
  bool m_has_position = false;
};

/// Lexer-level error: unrecognised character, malformed numeric literal,
/// etc. Raised before any grammar rule could match.
class lexical_error : public parse_error {
public:
  using parse_error::parse_error;
};

/// Grammar-level error: unexpected token, missing closing
/// paren/brace/bracket, premature end of input.
class syntax_error : public parse_error {
public:
  using parse_error::parse_error;
};

/// An identifier was used where typing forces a tensor (e.g. inside
/// `trace(...)`), but no tensor by that name has been declared.
///
/// @note Currently **unreachable from the parser** — every bare
/// identifier in scalar position is implicitly declared as a scalar
/// (see `symbol_table::get_or_declare_scalar`), so the "undeclared
/// identifier in tensor position" path is never taken. Pinned by
/// `ParserErrorCoverage.UnknownSymbolErrorIsUnreachableFromParser`.
/// The class is preserved for a potential future strict mode that
/// disables implicit scalar declaration, and is exercised directly
/// at the constructor level by `ParseError.UnknownSymbolErrorCarriesName`.
class unknown_symbol_error : public parse_error {
public:
  unknown_symbol_error(std::string name, std::size_t byte_offset,
                       std::string_view source);

  [[nodiscard]] std::string const &name() const noexcept { return m_name; }

private:
  std::string m_name;
};

/// A function name appeared in call position but is not in the
/// parser's known-functions registry.
class unknown_function_error : public parse_error {
public:
  unknown_function_error(std::string name, std::size_t byte_offset,
                         std::string_view source);

  [[nodiscard]] std::string const &name() const noexcept { return m_name; }

private:
  std::string m_name;
};

/// A function was called with the wrong number of arguments.
class arity_error : public parse_error {
public:
  arity_error(std::string function_name, std::size_t expected_arity,
              std::size_t actual_arity, std::size_t byte_offset,
              std::string_view source);

  [[nodiscard]] std::string const &function() const noexcept {
    return m_function;
  }
  [[nodiscard]] std::size_t expected_arity() const noexcept {
    return m_expected;
  }
  [[nodiscard]] std::size_t actual_arity() const noexcept { return m_actual; }

private:
  std::string m_function;
  std::size_t m_expected;
  std::size_t m_actual;
};

/// An operator was applied to operand types it does not support
/// (e.g. `sin(A)` where `A` is a tensor, or `tensor + scalar` which is
/// not in the codebase's `tag_invoke` surface).
class type_mismatch_error : public parse_error {
public:
  using parse_error::parse_error;
};

/// A tensor was re-declared with a different rank or dim than its
/// prior declaration. Raised by `symbol_table::get_or_declare_tensor`.
class redeclaration_error : public parse_error {
public:
  using parse_error::parse_error;
};

/// An identifier was used as both a scalar and a tensor (in either
/// order). Raised by `symbol_table`.
class type_collision_error : public parse_error {
public:
  using parse_error::parse_error;
};

} // namespace numsim::cas::parser

#endif // NUMSIM_CAS_PARSER_PARSE_ERROR_H
