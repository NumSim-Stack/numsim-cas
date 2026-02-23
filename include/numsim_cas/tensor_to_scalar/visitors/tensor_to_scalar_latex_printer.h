#ifndef TENSOR_TO_SCALAR_LATEX_PRINTER_H
#define TENSOR_TO_SCALAR_LATEX_PRINTER_H

#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>
#include <numsim_cas/tensor_to_scalar/visitors/tensor_to_scalar_printer.h>

#include <algorithm>
#include <numsim_cas/core/print_mul_fractions.h>
#include <numsim_cas/latex_printer_base.h>
#include <numsim_cas/tensor/sequence.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar_add.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar_mul.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_domain_traits.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_io.h>

namespace numsim::cas {

template <typename StreamType>
class tensor_to_scalar_latex_printer final
    : public tensor_to_scalar_visitor_const_t,
      public latex_printer_base<tensor_to_scalar_latex_printer<StreamType>,
                                StreamType> {
public:
  using base_visitor =
      latex_printer_base<tensor_to_scalar_latex_printer<StreamType>,
                         StreamType>;
  friend base_visitor;
  using base_visitor::begin;
  using base_visitor::end;
  using base_visitor::print_unary;

  explicit tensor_to_scalar_latex_printer(
      StreamType &out, latex_config const &cfg = latex_config::default_config())
      : base_visitor(out, cfg) {}

  tensor_to_scalar_latex_printer(tensor_to_scalar_latex_printer const &) =
      delete;
  tensor_to_scalar_latex_printer(tensor_to_scalar_latex_printer &&) = delete;
  const tensor_to_scalar_latex_printer &
  operator=(tensor_to_scalar_latex_printer const &) = delete;

  auto apply(expression_holder<tensor_to_scalar_expression> const &expr,
             Precedence parent_precedence = Precedence::None) {
    if (expr.is_valid()) {
      m_parent_precedence = parent_precedence;
      static_cast<const tensor_to_scalar_visitable_t &>(expr.get())
          .accept(static_cast<tensor_to_scalar_visitor_const_t &>(*this));
    }
  }

  void operator()(tensor_trace const &visitable) override {
    print_unary("\\operatorname{tr}", visitable);
  }

  void operator()(tensor_dot const &visitable) override {
    // dot(A) = A:A in LaTeX
    m_out << "\\operatorname{dot}\\left(";
    apply(visitable.expr());
    m_out << "\\right)";
  }

  void operator()(tensor_norm const &visitable) override {
    m_out << "\\left\\|";
    apply(visitable.expr());
    m_out << "\\right\\|";
  }

  void operator()(tensor_det const &visitable) override {
    print_unary("\\det", visitable);
  }

  void operator()(tensor_to_scalar_negative const &visitable) override {
    constexpr auto precedence{Precedence::Unary};
    m_out << "-";
    apply(visitable.expr(), precedence);
  }

  void operator()(tensor_to_scalar_mul const &visitable) override {
    using traits = domain_traits<tensor_to_scalar_expression>;
    const auto parent_precedence{m_parent_precedence};

    auto [num, denom] = partition_mul_fractions<traits>(visitable.symbol_map());

    // scalar-like children first for consistent ordering
    std::stable_partition(num.begin(), num.end(), [](auto const &c) {
      return detail::is_scalar_like(c);
    });

    if (!denom.empty()) {
      // \frac{num}{denom}
      std::stable_partition(denom.begin(), denom.end(), [](auto const &e) {
        return detail::is_scalar_like(e.base);
      });

      m_out << "\\frac{";

      bool first = true;
      if (visitable.coeff().is_valid()) {
        apply(visitable.coeff(), Precedence::None);
        first = false;
      }
      for (auto &child : num) {
        if (!first)
          m_out << " \\cdot ";
        apply(child, Precedence::None);
        first = false;
      }
      if (first) {
        m_out << "1";
      }

      m_out << "}{";

      bool dfirst = true;
      for (auto &entry : denom) {
        if (!dfirst)
          m_out << " \\cdot ";
        if (entry.pos_exponent != scalar_number{1}) {
          m_out << "{";
          apply(entry.base, Precedence::None);
          m_out << "}^{";
          m_out << entry.pos_exponent;
          m_out << "}";
        } else {
          apply(entry.base, Precedence::None);
        }
        dfirst = false;
      }

      m_out << "}";
    } else {
      // no denominator: a \cdot b
      constexpr auto precedence{Precedence::Multiplication};
      begin(precedence, parent_precedence);

      bool first = true;
      if (visitable.coeff().is_valid()) {
        apply(visitable.coeff(), precedence);
        first = false;
      }
      for (auto &child : num) {
        if (!first)
          m_out << " \\cdot ";
        apply(child, precedence);
        first = false;
      }

      end(precedence, parent_precedence);
    }
  }

  void operator()(tensor_to_scalar_log const &visitable) override {
    print_unary("\\ln", visitable);
  }

  void operator()(tensor_to_scalar_exp const &visitable) override {
    print_unary("\\exp", visitable);
  }

  void operator()(tensor_to_scalar_sqrt const &visitable) override {
    m_out << "\\sqrt{";
    apply(visitable.expr());
    m_out << "}";
  }

  void operator()(tensor_to_scalar_add const &visitable) override {
    constexpr auto precedence{Precedence::Addition};
    const auto parent_precedence{m_parent_precedence};
    begin(precedence, parent_precedence);

    using expr_t = expression_holder<tensor_to_scalar_expression>;
    std::vector<expr_t> children;
    children.reserve(visitable.symbol_map().size());
    for (auto &child : visitable.symbol_map() | std::views::values) {
      children.push_back(child);
    }
    std::stable_partition(children.begin(), children.end(), [](auto const &c) {
      return detail::is_scalar_like(c);
    });

    bool first{false};
    if (visitable.coeff().is_valid()) {
      apply(visitable.coeff(), precedence);
      first = true;
    }
    for (auto &child : children) {
      if (first && !is_same<tensor_to_scalar_negative>(child)) {
        m_out << "+";
      }
      apply(child, precedence);
      first = true;
    }

    end(precedence, parent_precedence);
  }

  void operator()(tensor_to_scalar_pow const &visitable) override {
    m_out << "{";
    apply(visitable.expr_lhs(), Precedence::Unary);
    m_out << "}^{";
    apply(visitable.expr_rhs());
    m_out << "}";
  }

  void operator()(tensor_inner_product_to_scalar const &visitable) override {
    const auto &indices_lhs{visitable.indices_lhs()};
    const auto &indices_rhs{visitable.indices_rhs()};
    const auto parent_precedence{m_parent_precedence};

    if (indices_lhs == sequence{1, 2} && indices_rhs == sequence{1, 2}) {
      begin(Precedence::Multiplication, parent_precedence);
      apply(visitable.expr_lhs(), Precedence::Multiplication);
      m_out << " : ";
      apply(visitable.expr_rhs(), Precedence::Multiplication);
      end(Precedence::Multiplication, parent_precedence);
    } else if (indices_lhs == sequence{1, 2, 3, 4} &&
               indices_rhs == sequence{1, 2, 3, 4}) {
      begin(Precedence::Multiplication, parent_precedence);
      apply(visitable.expr_lhs(), Precedence::Multiplication);
      m_out << " :: ";
      apply(visitable.expr_rhs(), Precedence::Multiplication);
      end(Precedence::Multiplication, parent_precedence);
    } else {
      m_out << "\\operatorname{dot}\\left(";
      apply(visitable.expr_lhs(), Precedence::None);
      m_out << ", ";
      m_out << indices_lhs;
      m_out << ", ";
      apply(visitable.expr_rhs(), Precedence::None);
      m_out << ", ";
      m_out << indices_rhs;
      m_out << "\\right)";
    }
  }

  void
  operator()([[maybe_unused]] tensor_to_scalar_one const &visitable) override {
    m_out << "1";
  }

  void
  operator()([[maybe_unused]] tensor_to_scalar_zero const &visitable) override {
    m_out << "0";
  }

  void operator()([[maybe_unused]] tensor_to_scalar_scalar_wrapper const
                      &visitable) override {
    apply(visitable.expr(), m_parent_precedence);
  }

  template <class T> void operator()([[maybe_unused]] T const &visitable) {
    static_assert(sizeof(T) == 0, "tensor_to_scalar_latex_printer: missing "
                                  "overload for this node type");
  }

private:
  void apply(expression_holder<tensor_expression> const &expr,
             Precedence parent_precedence = Precedence::None);

  void apply(expression_holder<scalar_expression> const &expr,
             Precedence parent_precedence = Precedence::None);

  using base_visitor::m_config;
  using base_visitor::m_out;
  Precedence m_parent_precedence;
};

} // namespace numsim::cas

#endif // TENSOR_TO_SCALAR_LATEX_PRINTER_H
