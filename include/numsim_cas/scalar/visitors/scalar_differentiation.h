#ifndef SCALAR_DIFFERENTIATION_H
#define SCALAR_DIFFERENTIATION_H

#include <algorithm>
#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/diff.h>
#include <numsim_cas/core/operators.h>
#include <numsim_cas/printer_base.h>
#include <numsim_cas/scalar/scalar_all.h>
#include <numsim_cas/scalar/scalar_operators.h>
#include <numsim_cas/scalar/scalar_std.h>
#include <ranges>

namespace numsim::cas {
// Forward declare the customization used by numsim::cas::diff for
// scalar/scalar.
expression_holder<scalar_expression>
tag_invoke(detail::diff_fn, std::type_identity<scalar_expression>,
           std::type_identity<scalar_expression>,
           expression_holder<scalar_expression> const &,
           expression_holder<scalar_expression> const &);

} // namespace numsim::cas

namespace numsim::cas {

/**
 * @brief Symbolic differentiation visitor for scalar expressions.
 *
 * Computes \f$\frac{d}{d a} f\f$ where \f$a\f$ is the variable passed to the
 * constructor.
 *
 * The implementation applies standard differentiation rules plus the chain
 * rule: \f[ \frac{d}{da} F(u(a)) = F'(u(a)) \cdot \frac{du}{da}. \f]
 *
 */
class scalar_differentiation final : public scalar_visitor_const_t {
public:
  using expr_holder_t = expression_holder<scalar_expression>;

  /**
   * @brief Construct a differentiator w.r.t. a specific scalar variable.
   *
   * @param arg The variable with respect to which differentiation is
   performed.
   */
  scalar_differentiation(expr_holder_t const &arg) : m_arg(arg) {}

  scalar_differentiation(scalar_differentiation const &) = delete;
  scalar_differentiation(scalar_differentiation &&) = delete;
  const scalar_differentiation &
  operator=(scalar_differentiation const &) = delete;

  /**
   * @brief Differentiate an expression (const lvalue).
   * @param expr Expression to differentiate.
   * @return The symbolic derivative d(expr)/d(m_arg).
   */
  auto apply(expr_holder_t const &expr) { return apply_imp(expr); }

  /**
   * @brief Derivative of a scalar variable.
   *
   * d(arg)/d(arg) = 1, and d(other)/d(arg) = 0.
   *
   * @param visitable Scalar variable node.
   */
  void operator()(scalar const &visitable) {
    if (visitable.hash_value() == m_arg.get().hash_value()) {
      m_result = get_scalar_one();
    } else {
      m_result = get_scalar_zero();
    }
  }

  /**
   * @brief Derivative of a named scalar function.
   *
   * Computes the derivative of the function body and wraps it as a new
   * scalar_named_expression with name "d<name>".
   *
   * @param visitable Function node.
   */
  void operator()(scalar_named_expression const &visitable) {
    scalar_differentiation diff(m_arg);
    expr_holder_t result{diff.apply(visitable.expr())};
    if (result.is_valid()) {
      m_result =
          make_expression<scalar_named_expression>("d" + visitable.name(), result);
    }
  }

  /**
   * @brief Product rule for multiplication nodes.
   * For \f$f(a)=c\prod_{i=1}^{n} a_i(a)\f$:
   * \f[
   *   f'(a)=c\sum_{j=1}^{n}\left(a'_j(a)\prod_{\substack{i=1\\ i\neq j}}^{n}
   * a_i(a)\right). \f]
   * @param visitable Multiplication node.
   * TODO: just copy the vector and manipulate the current entry
   */
  void operator()(scalar_mul const &visitable) {
    expr_holder_t expr_result;
    for (auto &expr_out : visitable.hash_map() | std::views::values) {
      expr_holder_t expr_result_in;
      for (auto &expr_in : visitable.hash_map() | std::views::values) {
        if (expr_out == expr_in) {
          scalar_differentiation diff(m_arg);
          expr_result_in *= diff.apply(expr_in);
        } else {
          expr_result_in *= expr_in;
        }
      }
      expr_result += expr_result_in;
    }

    // NOTE: This assumes coeff() is constant w.r.t. m_arg
    if (visitable.coeff().is_valid()) {
      m_result = std::move(expr_result) * visitable.coeff();
    } else {
      m_result = std::move(expr_result);
    }
  }

  /**
   * @brief Summation rule for addition nodes.
   *    * For \f$f(a)=c+\sum_{i=1}^{n} a_i(a)\f$:
   * \f[
   *   f'(a)=\sum_{i=1}^{n} a'_i(a).
   * \f]
   *    * @param visitable Addition node.
   */
  void operator()([[maybe_unused]] scalar_add const &visitable) {
    expr_holder_t expr_result;
    for (auto &child : visitable.hash_map() | std::views::values) {
      scalar_differentiation diff(m_arg);
      expr_result += diff.apply(child);
    }
    m_result = std::move(expr_result);
  }

  /**
   * @brief Negation rule.
   *    * For \f$f(a)=-g(a)\f$:
   * \f[
   *   f'(a)=-g'(a).
   * \f]
   *    * @param visitable Negation node.
   */
  void operator()(scalar_negative const &visitable) {
    scalar_differentiation diff(m_arg);
    auto diff_expr{diff.apply(visitable.expr())};
    if (diff_expr.is_valid() || !is_same<scalar_zero>(diff_expr)) {
      m_result = -diff_expr;
    }
  }

  // /**
  //  * @brief Quotient rule.
  //  *    * For \f$f(a)=\frac{g(a)}{h(a)}\f$:
  //  * \f[
  //  *   f'(a)=\frac{g'(a)h(a)-g(a)h'(a)}{h(a)^2}.
  //  * \f]
  //  *    * @param visitable Division node.
  //  */
  // void operator()(scalar_div const &visitable) noexcept{
  //   auto g{visitable.expr_lhs()};
  //   auto h{visitable.expr_rhs()};
  //   scalar_differentiation diff(m_arg);
  //   auto dg{diff.apply(g)};
  //   auto dh{diff.apply(h)};
  //   m_result = (dg * h - g * dh) / (h * h);
  // }

  /**
   * @brief Derivative of a constant.
   * @param visitable Constant node.
   */
  void operator()([[maybe_unused]] scalar_constant const &visitable) {
    m_result = get_scalar_zero();
  }

  /**
   * @brief Derivative of \f$\tan(u)\f$.
   *    * \f[
   *   \frac{d}{da}\tan(u(a)) =
   * \sec^2(u(a))\,u'(a)=\frac{1}{\cos^2(u(a))}\,u'(a). \f]
   *    * @param visitable Tangent node.
   */
  void operator()([[maybe_unused]] scalar_tan const &visitable) {
    const auto &one{get_scalar_one()};
    m_result = pow(one / cos(visitable.expr()), 2);
    apply_inner_unary(visitable);
  }

  /**
   * @brief Derivative of \f$\sin(u)\f$.
   *    * \f[
   *   \frac{d}{da}\sin(u(a)) = \cos(u(a))\,u'(a).
   * \f]
   *    * @param visitable Sine node.
   */
  void operator()([[maybe_unused]] scalar_sin const &visitable) {
    m_result = cos(visitable.expr());
    apply_inner_unary(visitable);
  }

  /**
   * @brief Derivative of \f$\cos(u)\f$.
   *    * \f[
   *   \frac{d}{da}\cos(u(a)) = -\sin(u(a))\,u'(a).
   * \f]
   *    * @param visitable Cosine node.
   */
  void operator()([[maybe_unused]] scalar_cos const &visitable) {
    m_result = -sin(visitable.expr());
    apply_inner_unary(visitable);
  }

  /**
   * @brief Derivative of one is zero.
   * @param visitable One node.
   */
  void operator()([[maybe_unused]] scalar_one const &visitable) {
    m_result = get_scalar_zero();
  }

  /**
   * @brief Derivative of zero is zero.
   * @param visitable Zero node.
   */
  void operator()([[maybe_unused]] scalar_zero const &visitable) {
    m_result = get_scalar_zero();
  }

  /**
   * @brief Derivative of \f$\arctan(u)\f$.
   *    * \f[
   *   \frac{d}{da}\arctan(u(a)) = \frac{1}{1+u(a)^2}\,u'(a).
   * \f]
   *    * @param visitable Arctangent node.
   */
  void operator()([[maybe_unused]] scalar_atan const &visitable) {
    auto &one{get_scalar_one()};
    m_result = (one / (one + pow(visitable.expr(), 2)));
    apply_inner_unary(visitable);
  }

  /**
   * @brief Derivative of \f$\arcsin(u)\f$.
   *    * \f[
   *   \frac{d}{da}\arcsin(u(a)) = \frac{1}{\sqrt{1-u(a)^2}}\,u'(a).
   * \f]
   *    * @param visitable Arcsine node.
   */
  void operator()([[maybe_unused]] scalar_asin const &visitable) {
    auto &one{get_scalar_one()};
    m_result = (one / (sqrt(one - pow(visitable.expr(), 2))));
    apply_inner_unary(visitable);
  }

  /**
   * @brief Derivative of \f$\arccos(u)\f$.
   *    * \f[
   *   \frac{d}{da}\arccos(u(a)) = -\frac{1}{\sqrt{1-u(a)^2}}\,u'(a).
   * \f]
   *    * @param visitable Arccosine node.
   */
  void operator()([[maybe_unused]] scalar_acos const &visitable) {
    auto &one{get_scalar_one()};
    m_result = -(one / (sqrt(one - pow(visitable.expr(), 2))));
    apply_inner_unary(visitable);
  }

  /**
   * @brief Derivative of \f$\sqrt{u}\f$.
   *    * \f[
   *   \frac{d}{da}\sqrt{u(a)}=\frac{1}{2\sqrt{u(a)}}\,u'(a).
   * \f]
   *    * @param visitable Square-root node.
   */
  void operator()([[maybe_unused]] scalar_sqrt const &visitable) {
    auto &one{get_scalar_one()};
    m_result = one / (2 * m_expr);
    apply_inner_unary(visitable);
  }

  /**
   * @brief Derivative of \f$e^{u}\f$.
   *    * \f[
   *   \frac{d}{da}\exp(u(a))=\exp(u(a))\,u'(a).
   * \f]
   *    * @param visitable Exponential node.
   */
  void operator()([[maybe_unused]] scalar_exp const &visitable) {
    m_result = m_expr;
    apply_inner_unary(visitable);
  }

  /**
   * @brief Derivative of \f$g(a)^{h(a)}\f$.
   *    * Constant exponent case (when \f$h'(a)=0\f$):
   * \f[
   *   \frac{d}{da} g(a)^{h} = h\,g(a)^{h-1}\,g'(a).
   * \f]
   *    * General case:
   * \f[
   *   \frac{d}{da} g(a)^{h(a)} =
   *   g(a)^{h(a)}\left(h'(a)\log(g(a)) + h(a)\frac{g'(a)}{g(a)}\right)
   *   = g(a)^{h(a)-1}\left(h'(a)\log(g(a))\,g(a)+h(a)g'(a)\right).
   * \f]
   *    * @param visitable Power node.
   */
  void operator()([[maybe_unused]] scalar_pow const &visitable) {
    const auto &g{visitable.expr_lhs()};
    const auto &h{visitable.expr_rhs()};

    const auto &one{get_scalar_one()};

    auto dg{diff(g, m_arg)};
    auto dh{diff(h, m_arg)};

    if (is_same<scalar_zero>(dh)) {
      // h is constant (w.r.t. m_arg)
      m_result = h * pow(g, h - one) * dg;
    } else {
      // general case
      m_result = pow(g, h - one) * (h * dg + dh * log(g) * g);
    }
  }

  /**
   * @brief sign(u)' is treated as zero (non-differentiable at 0).
   * @param visitable Sign node.
   */
  void operator()([[maybe_unused]] scalar_sign const &visitable) {
    m_result = get_scalar_zero();
  }

  /**
   * @brief Derivative of \f$|u|\f$ (symbolic form).
   *    * \f[
   *   \frac{d}{da}|u(a)| = \frac{u(a)}{|u(a)|}\,u'(a),
   * \f]
   * undefined at \f$u(a)=0\f$ (the CAS keeps the symbolic form).
   *    * @param visitable Absolute-value node.
   */
  void operator()([[maybe_unused]] scalar_abs const &visitable) {
    m_result = visitable.expr() / m_expr;
    apply_inner_unary(visitable);
  }

  /**
   * @brief Derivative of \f$\log(u)\f$.
   *    * \f[
   *   \frac{d}{da}\log(u(a))=\frac{1}{u(a)}\,u'(a).
   * \f]
   *    * @param visitable Logarithm node.
   */
  void operator()([[maybe_unused]] scalar_log const &visitable) {
    auto &one{get_scalar_one()};
    m_result = one / visitable.expr();
    apply_inner_unary(visitable);
  }

  void operator()([[maybe_unused]] scalar_rational const &visitable) {
    m_result = get_scalar_zero();
  }

  /**
   * @brief Default overload for safty reasons.
   */
  template <class T>
  void operator()([[maybe_unused]] T const &visitable) {
    static_assert(
        sizeof(T) == 0,
        "scalar_differentiation: missing overload for this node type");
  }

private:
  /**
   * @brief Apply chain rule for unary nodes.
   *    * If the current visitor has set \f$m\_result = F'(u)\f$, this
   * multiplies by \f$u'(a)\f$ to obtain \f$F'(u(a))\,u'(a)\f$.
   *    * @tparam T Unary node type exposing expr().
   * @param unary Unary node.
   */
  template <typename T> void apply_inner_unary(T const &unary) {
    scalar_differentiation diff(m_arg);
    auto inner{diff.apply(unary.expr())};
    if (inner.is_valid()) {
      m_result *= std::move(inner);
    }
  }

  /**
   * @brief Implementation of apply() overloads.
   * @param expr Expression to differentiate.
   * @return Derivative expression.
   */
  expr_holder_t apply_imp(expr_holder_t const &expr) {
    if (expr.is_valid()) {
      m_expr = expr;
      expr.get<scalar_visitable_t>().accept(*this);
      return m_result;
    } else {
      return get_scalar_zero();
    }
  }

  ///< Differentiation variable.
  expr_holder_t const &m_arg;
  ///< Currently visited expression node.
  expr_holder_t m_expr;
  ///< Accumulated result for current visit.
  expr_holder_t m_result;
};

} // namespace numsim::cas
#endif // SCALAR_DIFFERENTIATION_H
