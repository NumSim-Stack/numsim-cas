#ifndef SCALAR_PRINTER_H
#define SCALAR_PRINTER_H

#include <algorithm>
#include <numsim_cas/basic_functions.h>
#include <numsim_cas/printer_base.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_functions.h>
#include <ranges>
#include <vector>

namespace numsim::cas {
namespace detail {
struct scalar_pretty_printer {
  using expr_holder_t = expression_holder<scalar_expression>;
  inline bool operator()(expr_holder_t const &lhs,
                         expr_holder_t const &rhs) const noexcept {
    auto get_expr{[](expr_holder_t const &expr) noexcept {
      if (is_same<scalar_add>(expr)) {
        const auto &add{expr.get<scalar_add>()};
        if (add.hash_map().size() == 1) {
          return add.hash_map().begin()->second;
        }
      }

      if (is_same<scalar_mul>(expr)) {
        const auto &add{expr.get<scalar_mul>()};
        if (add.hash_map().size() == 1) {
          return add.hash_map().begin()->second;
        }
      }
      return expr;
    }};

    return get_expr(lhs) < get_expr(rhs);
  }
};
} // namespace detail

/**
 * @brief Class for printing scalar expressions with correct precedence and
 * formatting.
 *
 * @tparam StreamType The type of the output stream.
 */
template <typename StreamType>
class scalar_printer final
    : public scalar_visitor_const_t,
      public printer_base<scalar_printer<StreamType>, StreamType> {
public:
  using base = printer_base<scalar_printer<StreamType>, StreamType>;
  using expr_t = expression_holder<scalar_expression>;
  using base_visitor = scalar_visitor_const_t;
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
  auto apply(expression_holder<scalar_expression> const &expr,
             [[maybe_unused]] Precedence parent_precedence =
                 Precedence::None) noexcept {
    if (expr.is_valid()) {
      m_parent_precedence = parent_precedence;
      static_cast<const scalar_visitable_t &>(expr.get())
          .accept(static_cast<base_visitor &>(*this));
      // std::visit([this, parent_precedence](
      //                auto &&arg) { (*this)(arg, parent_precedence); },
      //            *expr);
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
  void operator()(scalar const &visitable/*,
                  [[maybe_unused]] Precedence parent_precedence*/) noexcept {
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
  void operator()(scalar_function const &visitable/*,
                  [[maybe_unused]] Precedence parent_precedence*/) noexcept {
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
  void operator()([[maybe_unused]] scalar_constant const &visitable) noexcept {
    m_out << visitable.value();
  }

  /**
   * @brief Prints a scalar "1" value.
   *
   * @param visitable The scalar "1" value to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()([[maybe_unused]] scalar_one const &visitable) noexcept {
    m_out << "1";
  }

  /**
   * @brief Prints a scalar "0" value.
   *
   * @param visitable The scalar "0" value to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()([[maybe_unused]] scalar_zero const &visitable) noexcept {
    m_out << "0";
  }

  /// ------------ scalar basic operators ------------
  /**
   * @brief Prints a scalar multiplication expression.
   *
   * @param visitable The scalar multiplication expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_mul const &visitable) noexcept {
    constexpr auto precedence{Precedence::Multiplication};
    const auto parent_precedence{m_parent_precedence};

    begin(precedence, parent_precedence);
    const auto values{visitable.hash_map() | std::views::values};
    std::vector<expr_t> power;
    std::vector<expr_t> numerator, denominator;
    power.reserve(visitable.hash_map().size());
    numerator.reserve(visitable.hash_map().size());
    denominator.reserve(visitable.hash_map().size());
    std::map<expr_t, expr_t, detail::scalar_pretty_printer> sorted_map;

    std::for_each(std::begin(values), std::end(values),
                  [&](auto &expr) { sorted_map[expr] = expr; });

    std::for_each(std::begin(values), std::end(values), [&](const auto &expr) {
      if (auto power_expr{is_same_r<scalar_pow>(expr)}) {
        const auto &expr_rhs{power_expr->get().expr_rhs()};
        if (auto expr_neg{is_same_r<scalar_negative>(expr_rhs)}) {
          if (is_scalar_constant(expr_neg->get().expr())) {
            power.push_back(expr);
            numerator.push_back(power_expr->get().expr_lhs());
            denominator.push_back(expr_neg->get().expr());
          }
        }
      }
    });

    std::for_each(std::begin(power), std::end(power),
                  [&](const auto &expr) { sorted_map.erase(expr); });

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

    if (!power.empty()) {
      m_out << "/";
      for (std::size_t i{0}; i < power.size(); ++i) {
        if (const auto number{get_scalar_number(denominator[i])}) {
          if (number != 1) {
            m_out << "pow(";
            apply(numerator[i]);
            m_out << ",";
            apply(denominator[i]);
            m_out << ")";
          } else {
            apply(numerator[i], Precedence::Division_RHS);
          }
        }
      }
    }

    end(precedence, parent_precedence);
  }

  /**
   * @brief Prints a scalar addition expression.
   *
   * @param visitable The scalar addition expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_add const &visitable) noexcept {
    constexpr auto precedence{Precedence::Addition};
    const auto parent_precedence{m_parent_precedence};
    begin(precedence, parent_precedence);
    const auto values{visitable.hash_map_values()};
    std::map<expr_t, expr_t, detail::scalar_pretty_printer> sorted_map;
    std::for_each(std::begin(values), std::end(values),
                  [&](const auto &expr) { sorted_map[expr] = expr; });

    bool first{false};
    if (visitable.coeff().is_valid()) {
      apply(visitable.coeff(), precedence);
      first = true;
    }

    for (auto &child : sorted_map | std::views::values) {
      if (first && !is_same<scalar_negative>(child)) {
        m_out << "+";
      }
      apply(child, precedence);
      first = true;
    }
    end(precedence, parent_precedence);
  }

  // /**
  //  * @brief Prints a scalar division expression.
  //  *
  //  * @param visitable The scalar division expression to be printed.
  //  * @param parent_precedence The precedence of the parent expression.
  //  */
  // void operator()(scalar_div const &visitable/*,
  //                 Precedence parent_precedence*/) noexcept {
  //   constexpr auto precedence{Precedence::Multiplication};
  //   const auto parent_precedence{m_parent_precedence};

  //   begin(precedence, parent_precedence);
  //   apply(visitable.expr_lhs(), Precedence::Division_LHS);
  //   m_out << "/";
  //   apply(visitable.expr_rhs(), Precedence::Division_RHS);
  //   end(precedence, parent_precedence);
  // }

  /**
   * @brief Prints a rational number.
   *
   * @param visitable The scalar division expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_rational const &visitable) noexcept {
    constexpr auto precedence{Precedence::Multiplication};
    const auto parent_precedence{m_parent_precedence};

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
  void operator()(scalar_negative const &visitable/*,
                  Precedence parent_precedence*/) noexcept {
    constexpr auto precedence{Precedence::Negative};
    const auto parent_precedence{m_parent_precedence};

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
  void operator()(scalar_log const &visitable/*,
                  [[maybe_unused]] Precedence parent_precedence*/) noexcept {
    print_unary("log", visitable);
  }

  /**
   * @brief Prints a scalar square root expression.
   *
   * @param visitable The scalar square root expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_sqrt const &visitable/*,
                  [[maybe_unused]] Precedence parent_precedence*/) noexcept {
    print_unary("sqrt", visitable);
  }

  /**
   * @brief Prints a scalar exponential expression.
   *
   * @param visitable The scalar exponential expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_exp const &visitable/*,
                  [[maybe_unused]] Precedence parent_precedence*/) noexcept {
    print_unary("exp", visitable);
  }

  /**
   * @brief Prints a scalar sign expression.
   *
   * @param visitable The scalar sign expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_sign const &visitable/*,
                  [[maybe_unused]] Precedence parent_precedence*/) noexcept {
    print_unary("sign", visitable);
  }

  /**
   * @brief Prints a scalar absolute value expression.
   *
   * @param visitable The scalar absolute value expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_abs const &visitable/*,
                  [[maybe_unused]] Precedence parent_precedence*/) noexcept {
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
  void operator()(scalar_pow const &visitable/*,
                  [[maybe_unused]] Precedence parent_precedence*/) noexcept {
    const auto &lhs{visitable.expr_lhs()};
    const auto &rhs{visitable.expr_rhs()};
    m_out << "pow(";
    apply(lhs);
    m_out << ",";
    apply(rhs);
    m_out << ")";
  }

  /// ------------ scalar trigonometric functions ------------
  /**
   * @brief Prints a scalar tangent expression.
   *
   * @param visitable The scalar tangent expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_tan const &visitable/*,
                  [[maybe_unused]] Precedence parent_precedence*/) noexcept {
    print_unary("tan", visitable);
  }

  /**
   * @brief Prints a scalar sine expression.
   *
   * @param visitable The scalar sine expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_sin const &visitable/*,
                  [[maybe_unused]] Precedence parent_precedence*/) noexcept {
    print_unary("sin", visitable);
  }

  /**
   * @brief Prints a scalar cosine expression.
   *
   * @param visitable The scalar cosine expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_cos const &visitable/*,
                  [[maybe_unused]] Precedence parent_precedence*/) noexcept {
    print_unary("cos", visitable);
  }

  /**
   * @brief Prints a scalar arctangent expression.
   *
   * @param visitable The scalar arctangent expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_atan const &visitable/*,
                  [[maybe_unused]] Precedence parent_precedence*/) noexcept {
    print_unary("atan", visitable);
  }

  /**
   * @brief Prints a scalar arcsine expression.
   *
   * @param visitable The scalar arcsine expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_asin const &visitable/*,
                  [[maybe_unused]] Precedence parent_precedence*/) noexcept {
    print_unary("asin", visitable);
  }

  /**
   * @brief Prints a scalar arccosine expression.
   *
   * @param visitable The scalar arccosine expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_acos const &visitable/*,
                  [[maybe_unused]] Precedence parent_precedence*/) noexcept {
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
  Precedence m_parent_precedence{Precedence::None};
  using base::m_first_term; ///< First term of the expression to be printed
  using base::m_out;        ///< The output stream used for printing.
};

} // namespace numsim::cas
#endif // SCALAR_PRINTER_H
