#ifndef SCALAR_PRINTER_H
#define SCALAR_PRINTER_H

#include "../../numsim_cas_type_traits.h"
#include "../../printer_base.h"
#include <algorithm>
#include <ranges>
#include <variant>
#include <vector>

namespace numsim::cas {

/**
 * @brief Class for printing scalar expressions with correct precedence and
 * formatting.
 *
 * @tparam ValueType The type of the scalar values.
 * @tparam StreamType The type of the output stream.
 */
template <typename ValueType, typename StreamType>
class scalar_printer final
    : public printer_base<scalar_printer<ValueType, StreamType>, StreamType> {
public:
  using base = printer_base<scalar_printer<ValueType, StreamType>, StreamType>;
  using expr_t = expression_holder<scalar_expression<ValueType>>;
  using base::begin;
  using base::end;
  using base::print_unary;

  /**
   * @brief Constructor for scalar_printer.
   *
   * @param out The output stream to which expressions will be printed.
   */
  explicit scalar_printer(StreamType &out) : base(out) {}

  // Disable copy and move operations
  scalar_printer(scalar_printer const &) = delete;
  scalar_printer(scalar_printer &&) = delete;
  const scalar_printer &operator=(scalar_printer const &) = delete;

  /**
   * @brief Applies the printer to an expression.
   *
   * @param expr The scalar expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  auto apply(expression_holder<scalar_expression<ValueType>> const &expr,
             Precedence parent_precedence = Precedence::None) noexcept {
    if (expr.is_valid()) {
      std::visit([this, parent_precedence](
                     auto &&arg) { (*this)(arg, parent_precedence); },
                 *expr);
      m_first_term = false;
    }
  }

  /// ------------ scalar fundamentals ------------
  /**
   * @brief Prints a scalar value.
   *
   * @param visitable The scalar value to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    m_out << visitable.name();
  }

  /**
   * @brief Prints a scalar function.
   *
   * Prints the function name. If this is the first printed term, also prints an
   * assignment (" = ") followed by the function expression.
   *
   * @param visitable The scalar function to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_function<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    m_out << visitable.name();
    if (m_first_term) {
      m_out << " = ";
      apply(visitable.expr());
    }
  }

  /**
   * @brief Prints a scalar constant value.
   *
   * @param visitable The scalar constant value to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_constant<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    m_out << visitable();
  }

  /**
   * @brief Prints a scalar "1" value.
   *
   * @param visitable The scalar "1" value to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()([[maybe_unused]] scalar_one<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    m_out << "1";
  }

  /**
   * @brief Prints a scalar "0" value.
   *
   * @param visitable The scalar "0" value to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()([[maybe_unused]] scalar_zero<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    m_out << "0";
  }

  /// ------------ scalar basic operators ------------
  /**
   * @brief Prints a scalar multiplication expression.
   *
   * @param visitable The scalar multiplication expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_mul<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    constexpr auto precedence{Precedence::Multiplication};
    begin(precedence, parent_precedence);
    const auto values{visitable.hash_map() | std::views::values};
    std::map<expr_t, expr_t> sorted_map;
    std::for_each(std::begin(values), std::end(values),
                  [&](auto &expr) { sorted_map[expr] = expr; });

    bool first = true;
    if (visitable.coeff().is_valid()) {
      apply(visitable.coeff(), precedence);
      m_out << "*";
    }
    for (auto &child : sorted_map | std::views::values) {
      if (!first)
        m_out << "*";
      apply(child, precedence);
      first = false;
    }
    end(precedence, parent_precedence);
  }

  /**
   * @brief Prints a scalar addition expression.
   *
   * @param visitable The scalar addition expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_add<ValueType> const &visitable,
                  Precedence parent_precedence) noexcept {
    constexpr auto precedence{Precedence::Addition};
    begin(precedence, parent_precedence);
    const auto values{visitable.hash_map() | std::views::values};
    std::map<expr_t, expr_t> sorted_map;
    std::for_each(std::begin(values), std::end(values),
                  [&](auto &expr) { sorted_map[expr] = expr; });

    bool first{false};
    if (visitable.coeff().is_valid()) {
      apply(visitable.coeff(), precedence);
      first = true;
    }

    for (auto &child : sorted_map | std::views::values) {
      if (first && !is_same<scalar_negative<ValueType>>(child)) {
        m_out << "+";
      }
      apply(child, precedence);
      first = true;
    }

    end(precedence, parent_precedence);
  }

  /**
   * @brief Prints a scalar division expression.
   *
   * @param visitable The scalar division expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_div<ValueType> const &visitable,
                  Precedence parent_precedence) noexcept {
    constexpr auto precedence{Precedence::Multiplication};
    begin(precedence, parent_precedence);
    apply(visitable.expr_lhs(), Precedence::Division_LHS);
    m_out << "/";
    apply(visitable.expr_rhs(), Precedence::Division_RHS);
    end(precedence, parent_precedence);
  }

  /**
   * @brief Prints a neative scalar expression.
   *
   * @param visitable The negative scalar expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_negative<ValueType> const &visitable,
                  Precedence parent_precedence) noexcept {
    constexpr auto precedence{Precedence::Negative};

    m_out << "-";
    begin(precedence, parent_precedence);
    apply(visitable.expr(), precedence);
    end(precedence, parent_precedence);
  }

  /// ------------ scalar special math functions ------------
  /**
   * @brief Prints a scalar log expression.
   *
   * @param visitable The scalar log expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_log<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    print_unary("log", visitable);
  }

  /**
   * @brief Prints a scalar square root expression.
   *
   * @param visitable The scalar square root expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_sqrt<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    print_unary("sqrt", visitable);
  }

  /**
   * @brief Prints a scalar exponential expression.
   *
   * @param visitable The scalar exponential expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_exp<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    print_unary("exp", visitable);
  }

  /**
   * @brief Prints a scalar sign expression.
   *
   * @param visitable The scalar sign expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_sign<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    print_unary("sign", visitable);
  }

  /**
   * @brief Prints a scalar absolute value expression.
   *
   * @param visitable The scalar absolute value expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_abs<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    print_unary("abs", visitable);
  }

  /**
   * @brief Prints a scalar power expression.
   *
   * Formats the expression as pow(lhs, rhs).
   *
   * @param visitable The scalar power expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_pow<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    m_out << "pow(";
    apply(visitable.expr_lhs());
    m_out << ",";
    apply(visitable.expr_rhs());
    m_out << ")";
  }

  /// ------------ scalar trigonometric functions ------------
  /**
   * @brief Prints a scalar tangent expression.
   *
   * @param visitable The scalar tangent expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_tan<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    print_unary("tan", visitable);
  }

  /**
   * @brief Prints a scalar sine expression.
   *
   * @param visitable The scalar sine expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_sin<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    print_unary("sin", visitable);
  }

  /**
   * @brief Prints a scalar cosine expression.
   *
   * @param visitable The scalar cosine expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_cos<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    print_unary("cos", visitable);
  }

  /**
   * @brief Prints a scalar arctangent expression.
   *
   * @param visitable The scalar arctangent expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_atan<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    print_unary("atan", visitable);
  }

  /**
   * @brief Prints a scalar arcsine expression.
   *
   * @param visitable The scalar arcsine expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_asin<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    print_unary("asin", visitable);
  }

  /**
   * @brief Prints a scalar arccosine expression.
   *
   * @param visitable The scalar arccosine expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_acos<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) noexcept {
    print_unary("acos", visitable);
  }

  /**
   * @brief Default overload for safty reasons.
   */
  template <class T> void operator()([[maybe_unused]] T const &visitable) {
    static_assert(sizeof(T) == 0,
                  "scalar_printer: missing overload for this node type");
  }

private:
  using base::m_first_term; ///< First term of the expression to be printed
  using base::m_out;        ///< The output stream used for printing.
};

} // namespace numsim::cas
#endif // SCALAR_PRINTER_H
