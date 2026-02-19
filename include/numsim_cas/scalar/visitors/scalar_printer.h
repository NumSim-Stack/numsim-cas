#ifndef SCALAR_PRINTER_H
#define SCALAR_PRINTER_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/printer_base.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_functions.h>

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

      if (is_same<scalar_pow>(expr)) {
        const auto &pow{expr.get<scalar_pow>()};
        // if (is_same<scalar_constant>(pow.expr_rhs()) ||
        //     is_same<scalar_one>(pow.expr_rhs())) {
        return pow.expr_lhs();
        //}
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
  void apply(expression_holder<scalar_expression> const &expr,
             [[maybe_unused]] Precedence parent_precedence =
                 Precedence::None);

  /// ------------ scalar fundamentals ------------
  /**
   * @brief Prints a scalar value.
   *
   * @param visitable The scalar value to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/);


  /**
   * @brief Prints a scalar function.
   *
   * Prints the function name. If this is the first printed term, also prints an
   * assignment (" = ") followed by the function expression.
   *
   * @param visitable The scalar function to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_named_expression const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/);


  /**
   * @brief Prints a scalar constant value.
   *
   * @param visitable The scalar constant value to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()([[maybe_unused]] scalar_constant const &visitable);

  /**
   * @brief Prints a scalar "1" value.
   *
   * @param visitable The scalar "1" value to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()([[maybe_unused]] scalar_one const &visitable);

  /**
   * @brief Prints a scalar "0" value.
   *
   * @param visitable The scalar "0" value to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()([[maybe_unused]] scalar_zero const &visitable);

  /// ------------ scalar basic operators ------------
  /**
   * @brief Prints a scalar multiplication expression.
   *
   * @param visitable The scalar multiplication expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_mul const &visitable);

  /**
   * @brief Prints a scalar addition expression.
   *
   * @param visitable The scalar addition expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_add const &visitable);

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
  void operator()(scalar_rational const &visitable);

  /**
   * @brief Prints a neative scalar expression.
   *
   * @param visitable The negative scalar expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_negative const &visitable/*,
                   Precedence parent_precedence*/);


  /// ------------ scalar special math functions ------------
  /**
   * @brief Prints a scalar log expression.
   *
   * @param visitable The scalar log expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_log const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/);


  /**
   * @brief Prints a scalar square root expression.
   *
   * @param visitable The scalar square root expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_sqrt const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/);


  /**
   * @brief Prints a scalar exponential expression.
   *
   * @param visitable The scalar exponential expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_exp const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/);


  /**
   * @brief Prints a scalar sign expression.
   *
   * @param visitable The scalar sign expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_sign const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/);


  /**
   * @brief Prints a scalar absolute value expression.
   *
   * @param visitable The scalar absolute value expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_abs const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/);


  /**
   * @brief Prints a scalar power expression.
   *
   * Formats the expression as pow(lhs, rhs).
   *
   * @param visitable The scalar power expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_pow const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/);


  /// ------------ scalar trigonometric functions ------------
  /**
   * @brief Prints a scalar tangent expression.
   *
   * @param visitable The scalar tangent expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_tan const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/);


  /**
   * @brief Prints a scalar sine expression.
   *
   * @param visitable The scalar sine expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_sin const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/);


  /**
   * @brief Prints a scalar cosine expression.
   *
   * @param visitable The scalar cosine expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_cos const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/);


  /**
   * @brief Prints a scalar arctangent expression.
   *
   * @param visitable The scalar arctangent expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_atan const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/);


  /**
   * @brief Prints a scalar arcsine expression.
   *
   * @param visitable The scalar arcsine expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_asin const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/);


  /**
   * @brief Prints a scalar arccosine expression.
   *
   * @param visitable The scalar arccosine expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(scalar_acos const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/);


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
