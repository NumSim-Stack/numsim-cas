#ifndef TENSOR_TO_SCALAR_DIFFERENTIATION_H
#define TENSOR_TO_SCALAR_DIFFERENTIATION_H

#include "../../numsim_cas_type_traits.h"
#include "../../operators.h"
#include "../../printer_base.h"
#include "../../scalar/visitors/scalar_printer.h"
#include "../../tensor/tensor_functions_fwd.h"
#include "../../tensor/visitors/tensor_printer.h"
#include "../tensor_to_scalar_std.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric> // std::iota
#include <ranges>  // std::views (if used elsewhere)
#include <string>
#include <variant>
#include <vector>

namespace numsim::cas {

/**
 * @file tensor_to_scalar_differentiation.h
 * @brief Symbolic differentiation of tensor-to-scalar expressions w.r.t. a
 * tensor argument.
 *
 * @details
 * This visitor computes the (tensor) derivative of a scalar-valued tensor
 * expression \f$ f(\mathbf{X}) \f$ with respect to a tensor argument \f$
 * \mathbf{X} \f$.
 *
 * The result is a tensor expression with the same dimension and rank as \f$
 * \mathbf{X} \f$: \f[ \frac{\partial f}{\partial \mathbf{X}}. \f]
 *
 * Internally, index contractions are formed using `inner_product(...)` and
 * `sequence(...)`.
 */
template <typename ValueType> class tensor_to_scalar_differentiation {
public:
  using value_type = ValueType;
  using expr_type = expression_holder<tensor_to_scalar_expression<value_type>>;
  using result_expr_type = expression_holder<tensor_expression<value_type>>;

  /**
   * @brief Construct a differentiation visitor for derivatives w.r.t. @p arg.
   * @param arg Tensor variable \f$ \mathbf{X} \f$ with respect to which we
   * differentiate.
   *
   * @note An identity-like tensor \f$ \delta_{ij} \f$ is created with the
   * dimension of @p arg.
   */
  explicit tensor_to_scalar_differentiation(result_expr_type const &arg)
      : m_arg(arg) {
    I = make_expression<kronecker_delta<ValueType>>(arg.get().dim());
  }

  tensor_to_scalar_differentiation(tensor_to_scalar_differentiation const &) =
      delete;
  tensor_to_scalar_differentiation(tensor_to_scalar_differentiation &&) =
      delete;
  tensor_to_scalar_differentiation const &
  operator=(tensor_to_scalar_differentiation const &) = delete;

  /**
   * @brief Differentiate a tensor-to-scalar expression w.r.t. the stored tensor
   * argument.
   * @param expr The expression \f$ f(\mathbf{X}) \f$.
   * @return The tensor derivative \f$ \partial f / \partial \mathbf{X} \f$.
   *
   * @details
   * If the expression does not depend on \f$ \mathbf{X} \f$ (or differentiation
   * yields no valid result), a zero tensor is returned.
   */
  [[nodiscard]] result_expr_type apply(expr_type const &expr) {
    if (expr.is_valid()) {
      m_expr = expr;
      std::visit([this](auto &&arg) { (*this)(arg); }, *expr);
    }

    if (!m_result.is_valid()) {
      return make_expression<tensor_zero<value_type>>(m_arg.get().dim(),
                                                      m_arg.get().rank());
    }
    return m_result;
  }

  /**
   * @brief Trace rule.
   *
   * @details For \f$ f(\mathbf{A}) = \mathrm{tr}(\mathbf{A}) \f$:
   * \f[
   * \mathrm{tr}(\mathbf{A}) = \delta_{ij} A_{ij},
   * \qquad
   * \frac{\partial}{\partial \mathbf{X}}\mathrm{tr}(\mathbf{A})
   * = \delta_{ij}\,\frac{\partial A_{ij}}{\partial \mathbf{X}}.
   * \f]
   */
  void operator()(tensor_trace<ValueType> const &visitable) {
    auto dA = diff(visitable.expr(), m_arg);
    if (!dA.is_valid()) {
      return; // zero
    }
    m_result = inner_product(I, sequence{1, 2}, std::move(dA), sequence{1, 2});
  }

  /**
   * @brief Dot rule (self-dot / Frobenius inner product with itself).
   *
   * @details If \f$ f(\mathbf{A}) = \mathbf{A}:\mathbf{A} = A_{i_1\ldots
   * i_r}A_{i_1\ldots i_r}\f$: \f[ \frac{\partial f}{\partial \mathbf{X}} =
   * 2\,A_{i_1\ldots i_r}\,\frac{\partial A_{i_1\ldots i_r}}{\partial
   * \mathbf{X}}. \f]
   */
  void operator()(tensor_dot<ValueType> const &visitable) {
    auto dA = diff(visitable.expr(), m_arg);
    if (!dA.is_valid()) {
      return; // zero
    }

    constexpr auto two{static_cast<value_type>(2)};
    sequence idx(visitable.expr().get().rank());
    std::iota(idx.begin(), idx.end(), 1);

    m_result = two * inner_product(visitable.expr(), idx, std::move(dA), idx);
  }

  /**
   * @brief Norm rule.
   *
   * @details For \f$ f(\mathbf{A}) = \|\mathbf{A}\| =
   * \sqrt{\mathbf{A}:\mathbf{A}} \f$: \f[ \frac{\partial}{\partial
   * \mathbf{X}}\|\mathbf{A}\| = \frac{\mathbf{A}:\frac{\partial
   * \mathbf{A}}{\partial \mathbf{X}}}{\|\mathbf{A}\|}. \f]
   */
  void operator()(tensor_norm<ValueType> const &visitable) {
    auto dA = diff(visitable.expr(), m_arg);
    if (!dA.is_valid()) {
      return; // zero
    }

    sequence idx(visitable.expr().get().rank());
    std::iota(idx.begin(), idx.end(), 1);

    m_result =
        inner_product(visitable.expr(), idx, std::move(dA), idx) / m_expr;
  }

  /**
   * @brief Determinant rule (2nd-order tensor).
   *
   * @details For \f$ f(\mathbf{A})=\det(\mathbf{A}) \f$:
   * \f[
   * \mathrm{d}\det(\mathbf{A})
   * = \det(\mathbf{A})\,\mathbf{A}^{-\mathsf{T}}:\mathrm{d}\mathbf{A},
   \qquad
   * \frac{\partial}{\partial \mathbf{X}}\det(\mathbf{A})
   * = \det(\mathbf{A})\,\mathbf{A}^{-\mathsf{T}}:\frac{\partial
   \mathbf{A}}{\partial \mathbf{X}}.
   * \f]
   */
  void operator()(tensor_det<ValueType> const &visitable) {
    auto dA = diff(visitable.expr(), m_arg);
    if (!dA.is_valid()) {
      return; // zero
    }
    m_result =
        m_expr * inner_product(inv(trans(visitable.expr())), sequence{1, 2},
                               std::move(dA), sequence{1, 2});
  }

  /**
   * @brief Negation rule.
   * @details \f$ \frac{\partial}{\partial \mathbf{X}}(-f)= -\frac{\partial
   * f}{\partial \mathbf{X}} \f$.
   */
  void operator()(tensor_to_scalar_negative<ValueType> const &visitable) {
    auto df = diff(visitable.expr(), m_arg);
    if (!df.is_valid()) {
      return; // zero
    }
    m_result = -std::move(df);
  }

  /**
   * @brief Product rule.
   *
   * @details For \f$ f(\mathbf{X}) = \prod_{i=1}^n a_i(\mathbf{X}) \f$:
   * \f[
   * \frac{\partial f}{\partial \mathbf{X}}
   * = \sum_{j=1}^{n}\left(\frac{\partial a_j}{\partial \mathbf{X}}
   * \prod_{\substack{i=1\\ i\neq j}}^{n} a_i\right).
   * \f]
   */
  void operator()(tensor_to_scalar_mul<ValueType> const &visitable) {
    auto const &factors = visitable.hash_map();

    // Start from explicit zero (avoids relying on invalid-state arithmetic).
    result_expr_type sum = make_expression<tensor_zero<value_type>>(
        m_arg.get().dim(), m_arg.get().rank());

    for (auto it_out = factors.begin(); it_out != factors.end(); ++it_out) {
      auto d_aj = diff(it_out->second, m_arg);
      if (!d_aj.is_valid() || is_same<tensor_zero<value_type>>(d_aj)) {
        continue; // contributes nothing
      }

      result_expr_type term = std::move(d_aj);
      for (auto it_in = factors.begin(); it_in != factors.end(); ++it_in) {
        if (it_in == it_out) {
          continue; // product over i != j
        }
        term = std::move(term) * it_in->second;
      }
      sum += std::move(term);
    }

    m_result = std::move(sum);
  }

  /**
   * @brief Summation rule.
   *
   * @details If \f$ f(\mathbf{X}) = \sum_{i=1}^n a_i(\mathbf{X}) \f$:
   * \f[
   * \frac{\partial f}{\partial \mathbf{X}} = \sum_{i=1}^n \frac{\partial
   * a_i}{\partial \mathbf{X}}. \f]
   */
  void operator()(tensor_to_scalar_add<ValueType> const &visitable) {
    result_expr_type sum = make_expression<tensor_zero<value_type>>(
        m_arg.get().dim(), m_arg.get().rank());

    for (auto &child : visitable.hash_map() | std::views::values) {
      auto d = diff(child, m_arg);
      if (d.is_valid()) {
        sum = std::move(sum) + std::move(d);
      }
    }
    m_result = std::move(sum);
  }

  /**
   * @brief Quotient rule.
   *
   * @details For \f$ f(\mathbf{X}) = g(\mathbf{X})/h(\mathbf{X})\f$:
   * \f[
   * \frac{\partial f}{\partial \mathbf{X}}
   * = \frac{g'(\mathbf{X})\,h(\mathbf{X}) -
   * g(\mathbf{X})\,h'(\mathbf{X})}{h(\mathbf{X})^2}. \f]
   *
   * @note If either derivative is not valid (no dependency), it is treated as
   * zero.
   */
  void operator()(tensor_to_scalar_div<ValueType> const &visitable) {
    auto g{visitable.expr_lhs()};
    auto h{visitable.expr_rhs()};

    auto dg{diff(g, m_arg)};
    auto dh{diff(h, m_arg)};

    if (!dg.is_valid()) {
      dg = make_expression<tensor_zero<value_type>>(m_arg.get().dim(),
                                                    m_arg.get().rank());
    }
    if (!dh.is_valid()) {
      dh = make_expression<tensor_zero<value_type>>(m_arg.get().dim(),
                                                    m_arg.get().rank());
    }

    m_result = (dg * h - g * dh) / (h * h);
  }

  /**
   * @brief Tensor-to-scalar power with tensor-to-scalar exponent.
   * @details
   * Let \f$g(\mathbf{X})\f$ and \f$h(\mathbf{X})\f$ be scalar-valued functions
   * and define \f[ f(\mathbf{X}) = g(\mathbf{X})^{h(\mathbf{X})}. \f] For the
   * differentiable branch (typically assuming \f$g(\mathbf{X}) > 0\f$ in the
   * real case), rewrite \f$f\f$ via the exponential: \f[ f(\mathbf{X}) =
   * \exp\!\big(h(\mathbf{X}) \ln(g(\mathbf{X}))\big). \f] Then the chain rule
   * gives: \f[ \frac{\partial f}{\partial \mathbf{X}} =
   * g(\mathbf{X})^{h(\mathbf{X})} \left( \frac{\partial h}{\partial \mathbf{X}}
   * \,\ln(g(\mathbf{X}))
   * + h(\mathbf{X}) \frac{1}{g(\mathbf{X})}\frac{\partial g}{\partial
   * \mathbf{X}} \right). \f]
   */
  void operator()(
      [[maybe_unused]] tensor_to_scalar_pow<ValueType> const &visitable) {
    const auto &g{visitable.expr_lhs()};
    const auto &h{visitable.expr_rhs()};

    auto one{make_expression<tensor_to_scalar_one<ValueType>>()};

    auto dg{diff(g, m_arg)};
    auto dh{diff(h, m_arg)};

    // If dh == 0 -> exponent constant w.r.t. X
    if (is_same<tensor_zero<value_type>>(dh)) {
      // df/dX = h * g^(h-1) * dg
      m_result = h * std::pow(g, h - one) * dg;
    } else {
      // df/dX = g^(h-1) * (h*dg + dh*log(g)*g)
      m_result = std::pow(g, h - one) * (h * dg + dh * std::log(g) * g);
    }
  }

  /**
   * @brief Power rule with scalar exponent.
   * @warning Not implemented yet.
   *
   * @details If exponent is a constant \f$ p \f$, then:
   * \f[
   * \frac{\partial}{\partial \mathbf{X}} g^p = p\,g^{p-1}\,g'.
   * \f]
   */
  void operator()([[maybe_unused]] tensor_to_scalar_pow_with_scalar_exponent<
                  ValueType> const &visitable) {
    auto dg{diff(visitable.expr_lhs(), m_arg)};
    if (!dg.is_valid()) {
      return; // zero
    }
    const auto &one{get_scalar_one<ValueType>()};
    m_result = visitable.expr_rhs() *
               std::pow(visitable.expr_lhs(), visitable.expr_rhs() - one) * dg;
  }

  /**
   * @brief Scalar multiplication: \f$ f = c\cdot g(\mathbf{X}) \f$.
   * @details \f$ \frac{\partial f}{\partial \mathbf{X}} = c\cdot \frac{\partial
   * g}{\partial \mathbf{X}} \f$.
   */
  void
  operator()(tensor_to_scalar_with_scalar_mul<ValueType> const &visitable) {
    auto dg{diff(visitable.expr_rhs(), m_arg)};
    if (!dg.is_valid()) {
      return; // zero
    }
    m_result = visitable.expr_lhs() * std::move(dg);
  }

  /**
   * @brief Scalar addition: \f$ f = c + g(\mathbf{X}) \f$.
   * @details \f$ \frac{\partial f}{\partial \mathbf{X}} = \frac{\partial
   * g}{\partial \mathbf{X}} \f$.
   */
  void
  operator()(tensor_to_scalar_with_scalar_add<ValueType> const &visitable) {
    m_result = diff(visitable.expr_rhs(), m_arg);
  }

  /**
   * @brief Division by scalar: \f$ f = g(\mathbf{X})/c \f$.
   * @details \f$ \frac{\partial f}{\partial \mathbf{X}} =
   * \frac{1}{c}\frac{\partial g}{\partial \mathbf{X}} \f$.
   */
  void
  operator()(tensor_to_scalar_with_scalar_div<ValueType> const &visitable) {
    auto dg{diff(visitable.expr_lhs(), m_arg)};
    if (!dg.is_valid()) {
      return; // zero
    }
    m_result = std::move(dg) / visitable.expr_rhs();
  }

  /**
   * @brief Division by tensor to scalar: \f$ f = c/g(\mathbf{X}) \f$.
   * @details \f$ \frac{\partial f}{\partial \mathbf{X}} =
   * -\frac{c}{g(\mathbf{X})^2}\frac{\partial g}{\partial \mathbf{X}} \f$.
   */
  void
  operator()([[maybe_unused]] scalar_with_tensor_to_scalar_div<ValueType> const
                 &visitable) {
    auto dg = diff(visitable.expr_rhs(), m_arg);
    if (!dg.is_valid()) {
      return; // zero
    }
    m_result = -visitable.expr_lhs() * dg /
               std::pow(visitable.expr_rhs(), static_cast<value_type>(2));
  }

  /**
   * @brief Differentiate a tensor inner product that yields a scalar.
   * @details
   * Let
   * \f[
   * f(\mathbf{X}) = \operatorname{inner}\!\Big(\mathbf{a}(\mathbf{X}),
   * \mathcal{I}_a,\; \mathbf{b}(\mathbf{X}), \mathcal{I}_b\Big), \f] where
   * \f$\mathcal{I}_a\f$ and \f$\mathcal{I}_b\f$ denote the index lists used for
   * contraction.
   *    * Using the product rule:
   * \f[
   * \frac{\partial f}{\partial \mathbf{X}} =
   * \operatorname{inner}\!\Big(\frac{\partial \mathbf{a}}{\partial \mathbf{X}},
   * \mathcal{I}_a,\; \mathbf{b}(\mathbf{X}), \mathcal{I}_b\Big)
   * +
   * \operatorname{inner}\!\Big(\mathbf{a}(\mathbf{X}), \mathcal{I}_a,\;
   * \frac{\partial \mathbf{b}}{\partial \mathbf{X}}, \mathcal{I}_b\Big).
   * \f]
   *    * If \f$\mathbf{a}\f$ (resp. \f$\mathbf{b}\f$) does not depend on
   * \f$\mathbf{X}\f$ then the corresponding derivative term is treated as zero.
   */
  void
  operator()([[maybe_unused]] tensor_inner_product_to_scalar<ValueType> const
                 &visitable) {
    auto da{diff(visitable.expr_lhs(), m_arg)};
    auto db{diff(visitable.expr_rhs(), m_arg)};
    sequence sequence_lhs{visitable.indices_lhs()};
    sequence sequence_rhs{visitable.indices_rhs()};
    std::ranges::for_each(sequence_lhs, [](auto &i) { i += 1; });
    std::ranges::for_each(sequence_rhs, [](auto &i) { i += 1; });
    if (da.is_valid()) {
      m_result =
          inner_product(da, sequence_lhs, visitable.expr_rhs(), sequence_rhs);
    }
    if (db.is_valid()) {
      m_result =
          std::move(m_result) +
          inner_product(visitable.expr_lhs(), sequence_lhs, db, sequence_rhs);
    }
  }

  /**
   * @brief Natural logarithm of a tensor-to-scalar expression.
   * @details
   * Let \f$ f(\mathbf{X}) = \ln(g(\mathbf{X})) \f$. Then
   * \f[
   * \frac{\partial f}{\partial \mathbf{X}}
   * = \frac{1}{g(\mathbf{X})}\,\frac{\partial g(\mathbf{X})}{\partial
   * \mathbf{X}}. \f]
   */
  void operator()(
      [[maybe_unused]] tensor_to_scalar_log<ValueType> const &visitable) {
    auto dg = diff(visitable.expr(), m_arg);
    if (!dg.is_valid()) {
      return; // zero
    }
    m_result = std::move(dg) / visitable.expr();
  }

  /**
   * @brief Tensor to scalar one
   *
   * @note Will be set to zero in apply function
   */
  void operator()(
      [[maybe_unused]] tensor_to_scalar_one<ValueType> const &visitable) {
    // set zero in apply function
  }

  /**
   * @brief Tensor to scalar zero
   *
   * @note Will be set to zero in apply function
   */
  void operator()(
      [[maybe_unused]] tensor_to_scalar_zero<ValueType> const &visitable) {
    // set zero in apply function
  }
  /**
   * @brief Scalar expression converted into a tensor_to_scalar expressiong
   *
   * @note Will be set to zero in apply function
   */
  void
  operator()([[maybe_unused]] tensor_to_scalar_scalar_wrapper<ValueType> const
                 &visitable) {
    // set zero in apply function
  }

  /**
   * @brief Default overload for safty reasons.
   */
  template <class T> void operator()([[maybe_unused]] T const &visitable) {
    static_assert(sizeof(T) == 0, "tensor_to_scalar_differentiation: missing "
                                  "overload for this node type");
  }

private:
  result_expr_type const &m_arg;
  result_expr_type m_result{nullptr};
  expr_type m_expr{nullptr};
  result_expr_type I{nullptr};
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_DIFFERENTIATION_H
