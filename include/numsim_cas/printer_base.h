#ifndef PRINTER_BASE_H
#define PRINTER_BASE_H

#include "numsim_cas_forward.h"
#include <algorithm>
#include <string_view>

namespace numsim::cas {
// Define a strongly-typed enum class for operation precedence
enum class Precedence {
  None,     // No precedence (for top-level calls)
  Addition, // Addition / Subtraction precedence
  Negative,
  Division_LHS,
  Multiplication, // Multiplication and Division precedence
  Division_RHS,
  Unary // Unary operations like negation
};

template <typename Derived, typename StreamType> class printer_base {
public:
  printer_base(StreamType &out) : m_out(out) {}

  void print(std::string_view out) { m_out << out; }

  template <typename ExprType>
  auto apply(expression_holder<ExprType> const &expr,
             Precedence parent_precedence = Precedence::None) {
    static_cast<Derived &>(*this).apply(expr, parent_precedence);
  }

protected:
  template <typename Stream, typename Container>
  void print_sequence(Stream &out, Container const &data, char spacer) {
    bool first{false};
    std::for_each(std::begin(data), std::end(data), [&](auto const &el) {
      if (first)
        out << spacer;
      out << el;
      first = true;
    });
  }

  void begin(Precedence current_precedence, Precedence parent_precedence) {
    if (current_precedence < parent_precedence)
      m_out << "(";
  }

  void end(Precedence current_precedence, Precedence parent_precedence) {
    if (current_precedence < parent_precedence)
      m_out << ")";
  }

  /**
   * @brief Prints a unary scalar operation.
   *
   * @param name The name of the unary operation.
   * @param visitable The scalar expression to which the unary operation
   * applies.
   * @param parent_precedence The precedence of the parent expression.
   */
  template <typename Visitable>
  void print_unary(std::string_view name, Visitable const &visitable) {
    m_out << name << "(";
    static_cast<Derived &>(*this).apply(visitable.expr());
    m_out << ")";
  }

  StreamType &m_out;
};

} // namespace numsim::cas

#endif // PRINTER_BASE_H
