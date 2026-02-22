#ifndef TENSOR_LATEX_PRINTER_H
#define TENSOR_LATEX_PRINTER_H

#include <numsim_cas/latex_printer_base.h>
#include <numsim_cas/scalar/scalar_functions.h>
#include <numsim_cas/scalar/visitors/scalar_latex_printer.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_io.h>

#include <algorithm>
#include <string>
#include <vector>

namespace numsim::cas {

template <typename StreamType>
class tensor_latex_printer final
    : public tensor_visitor_const_t,
      public latex_printer_base<tensor_latex_printer<StreamType>, StreamType> {
public:
  using base = latex_printer_base<tensor_latex_printer<StreamType>, StreamType>;
  using expr_t = expression_holder<tensor_expression>;
  using base_visitor = tensor_visitor_const_t;
  using base::begin;
  using base::end;
  using base::print_unary;

  explicit tensor_latex_printer(
      StreamType &out, latex_config const &cfg = latex_config::default_config())
      : base(out, cfg) {}

  tensor_latex_printer(tensor_latex_printer const &) = delete;
  tensor_latex_printer(tensor_latex_printer &&) = delete;
  const tensor_latex_printer &operator=(tensor_latex_printer const &) = delete;

  auto apply(expression_holder<tensor_expression> const &expr,
             Precedence parent_precedence = Precedence::None) {
    if (expr.is_valid()) {
      m_parent_precedence = parent_precedence;
      static_cast<const tensor_visitable_t &>(expr.get())
          .accept(static_cast<base_visitor &>(*this));
    }
  }

  void operator()(tensor const &visitable) override {
    m_out << m_config.format_tensor(visitable.name(), visitable.rank());
  }

  void operator()([[maybe_unused]] identity_tensor const &visitable) override {
    auto font = m_config.font_for_rank(visitable.rank());
    m_out << font << "{I}^{(" << visitable.rank() << ")}";
  }

  void operator()([[maybe_unused]] tensor_projector const &visitable) override {
    tensor_trace_print_visitor perm_trace_printer(visitable);
    auto label = perm_trace_printer.apply();
    auto font = m_config.font_for_rank(visitable.rank());

    // Extract subscript text from label like "P_dev" -> "dev"
    m_out << font << "{P}";
    if (auto pos = label.find('_'); pos != std::string::npos) {
      m_out << "_{\\mathrm{" << label.substr(pos + 1) << "}}";
    }
    m_out << "^{(" << visitable.rank() << ")}";
  }

  void operator()(tensor_add const &visitable) override {
    constexpr auto precedence{Precedence::Addition};
    const auto parent_precedence{m_parent_precedence};

    begin(precedence, parent_precedence);
    const auto values{visitable.symbol_map() | std::views::values};
    std::map<expr_t, expr_t> sorted_map;
    std::for_each(std::begin(values), std::end(values),
                  [&](auto &expr) { sorted_map[expr] = expr; });

    bool first{false};
    if (visitable.coeff().is_valid()) {
      apply(visitable.coeff(), precedence);
      first = true;
    }

    for (auto &child : sorted_map | std::views::values) {
      if (first && !is_same<tensor_negative>(child)) {
        m_out << "+";
      }
      apply(child, precedence);
      first = true;
    }

    end(precedence, parent_precedence);
  }

  void operator()(tensor_mul const &visitable) override {
    constexpr auto precedence{Precedence::Multiplication};
    const auto parent_precedence{m_parent_precedence};

    begin(precedence, parent_precedence);
    bool first = true;
    if (visitable.coeff().is_valid()) {
      apply(visitable.coeff(), precedence);
      first = false;
    }
    for (auto &child : visitable.data()) {
      if (!first)
        m_out << " \\cdot ";
      apply(child, precedence);
      first = false;
    }
    end(precedence, parent_precedence);
  }

  void operator()(tensor_negative const &visitable) override {
    constexpr auto precedence{Precedence::Negative};
    const auto parent_precedence{m_parent_precedence};

    m_out << "-";
    begin(precedence, parent_precedence);
    apply(visitable.expr(), precedence);
    end(precedence, parent_precedence);
  }

  void operator()(inner_product_wrapper const &visitable) override {
    constexpr auto precedence{Precedence::Multiplication};

    const auto &indices_lhs{visitable.indices_lhs()};
    const auto &indices_rhs{visitable.indices_rhs()};

    // Projector function notation: P:A -> dev(A), sym(A), vol(A), skew(A)
    if (indices_lhs == sequence{3, 4} && indices_rhs == sequence{1, 2} &&
        is_same<tensor_projector>(visitable.expr_lhs())) {
      auto const &proj = visitable.expr_lhs().template get<tensor_projector>();
      if (proj.acts_on_rank() == 2) {
        auto const &sp = proj.space();
        const char *fn = nullptr;
        if (std::holds_alternative<Symmetric>(sp.perm) &&
            std::holds_alternative<DeviatoricTag>(sp.trace))
          fn = "\\operatorname{dev}";
        else if (std::holds_alternative<Symmetric>(sp.perm) &&
                 std::holds_alternative<AnyTraceTag>(sp.trace))
          fn = "\\operatorname{sym}";
        else if (std::holds_alternative<Symmetric>(sp.perm) &&
                 std::holds_alternative<VolumetricTag>(sp.trace))
          fn = "\\operatorname{vol}";
        else if (std::holds_alternative<Skew>(sp.perm) &&
                 std::holds_alternative<AnyTraceTag>(sp.trace))
          fn = "\\operatorname{skew}";
        if (fn) {
          m_out << fn << "\\left(";
          apply(visitable.expr_rhs(), Precedence::None);
          m_out << "\\right)";
          return;
        }
      }
    }

    // single contraction
    const auto rank_lhs{call_tensor::rank(visitable.expr_lhs())};
    if (indices_lhs == sequence{rank_lhs} && indices_rhs == sequence{1}) {
      apply(visitable.expr_lhs(), precedence);
      m_out << " \\cdot ";
      apply(visitable.expr_rhs(), precedence);
      return;
    }

    // double contraction
    if (indices_lhs == sequence{rank_lhs - 1, rank_lhs} &&
        indices_rhs == sequence{1, 2}) {
      apply(visitable.expr_lhs(), precedence);
      m_out << " : ";
      apply(visitable.expr_rhs(), precedence);
      return;
    }

    // fourth contraction
    if (indices_lhs ==
            sequence{rank_lhs - 3, rank_lhs - 2, rank_lhs - 1, rank_lhs} &&
        indices_rhs == sequence{1, 2, 3, 4}) {
      apply(visitable.expr_lhs(), precedence);
      m_out << " :: ";
      apply(visitable.expr_rhs(), precedence);
      return;
    }

    m_out << "\\operatorname{inner}\\left(";
    apply(visitable.expr_lhs(), precedence);
    m_out << indices_lhs;
    m_out << ", ";
    apply(visitable.expr_rhs(), precedence);
    m_out << ", ";
    m_out << indices_rhs;
    m_out << "\\right)";
  }

  void operator()(basis_change_imp const &visitable) override {
    auto const &indices_temp = visitable.indices();
    if (indices_temp == sequence{2, 1}) {
      m_out << "{";
      apply(visitable.expr(), Precedence::None);
      m_out << "}^{\\mathrm{T}}";
    } else {
      m_out << "\\operatorname{permute}\\left(";
      apply(visitable.expr(), m_parent_precedence);
      m_out << ", ";
      m_out << visitable.indices();
      m_out << "\\right)";
    }
  }

  void operator()(outer_product_wrapper const &visitable) override {
    auto const &indices_temp_lhs = visitable.indices_lhs();
    auto const &indices_temp_rhs = visitable.indices_rhs();

    if (indices_temp_lhs == sequence{1, 4} &&
        indices_temp_rhs == sequence{2, 3}) {
      apply(visitable.expr_lhs(), Precedence::Multiplication);
      m_out << " \\underline{\\otimes} ";
      apply(visitable.expr_rhs(), Precedence::Multiplication);
    } else if (indices_temp_lhs == sequence{1, 3} &&
               indices_temp_rhs == sequence{2, 4}) {
      apply(visitable.expr_lhs(), Precedence::Multiplication);
      m_out << " \\bar{\\otimes} ";
      apply(visitable.expr_rhs(), Precedence::Multiplication);
    } else {
      m_out << "\\operatorname{outer}\\left(";
      apply(visitable.expr_lhs(), Precedence::None);
      m_out << ", ";
      m_out << indices_temp_lhs;
      m_out << ", ";
      apply(visitable.expr_rhs(), Precedence::None);
      m_out << ", ";
      m_out << indices_temp_rhs;
      m_out << "\\right)";
    }
  }

  void
  operator()([[maybe_unused]] simple_outer_product const &visitable) override {
    constexpr auto precedence{Precedence::Multiplication};
    const auto parent_precedence{m_parent_precedence};

    begin(precedence, parent_precedence);
    bool first = true;
    if (visitable.coeff().is_valid()) {
      apply(visitable.coeff(), precedence);
      m_out << " \\cdot ";
    }
    m_out << "\\operatorname{outer}\\left(";
    for (auto &child : visitable.data()) {
      if (!first)
        m_out << ",";
      apply(child, precedence);
      first = false;
    }
    m_out << "\\right)";
    end(precedence, parent_precedence);
  }

  void operator()([[maybe_unused]] kronecker_delta const &visitable) override {
    m_out << m_config.format_tensor("I", 2);
  }

  void operator()(tensor_scalar_mul const &visitable) override {
    constexpr auto precedence{Precedence::Multiplication};
    const auto parent_precedence{m_parent_precedence};

    begin(precedence, parent_precedence);

    using scalar_expr_t = expression_holder<scalar_expression>;
    scalar_latex_printer<StreamType> sp(m_out, m_config);

    // Special case: tensor * pow(base, -exp) -> tensor / pow(base, exp)
    if (auto pow_e = is_same_r<scalar_pow>(visitable.expr_lhs())) {
      const auto &p = pow_e->get();

      if (auto neg_e = is_same_r<scalar_negative>(p.expr_rhs())) {
        const auto &neg = neg_e->get();
        const scalar_expr_t &exp_pos = neg.expr();

        // \frac{tensor}{denom}
        m_out << "\\frac{";
        apply(visitable.expr_rhs(), Precedence::None);
        m_out << "}{";

        if (const auto number = get_scalar_number(exp_pos)) {
          if (*number == 1) {
            sp.apply(p.expr_lhs(), Precedence::None);
          } else {
            m_out << "{";
            sp.apply(p.expr_lhs(), Precedence::None);
            m_out << "}^{";
            m_out << *number;
            m_out << "}";
          }
        } else {
          m_out << "{";
          sp.apply(p.expr_lhs(), Precedence::None);
          m_out << "}^{";
          sp.apply(exp_pos);
          m_out << "}";
        }

        m_out << "}";
        end(precedence, parent_precedence);
        return;
      }
    }

    // scalar * tensor
    apply(visitable.expr_lhs(), precedence);
    m_out << " \\cdot ";
    apply(visitable.expr_rhs(), precedence);

    end(precedence, parent_precedence);
  }

  void operator()([[maybe_unused]] tensor_to_scalar_with_tensor_mul const
                      &visitable) override {
    constexpr auto precedence{Precedence::Multiplication};
    const auto parent_precedence{m_parent_precedence};

    begin(precedence, parent_precedence);
    apply(visitable.expr_rhs(), precedence);
    m_out << " \\cdot ";
    apply(visitable.expr_lhs(), precedence);
    end(precedence, parent_precedence);
  }

  void operator()(tensor_inv const &visitable) override {
    m_out << "{";
    apply(visitable.expr(), Precedence::None);
    m_out << "}^{-1}";
  }

  void operator()([[maybe_unused]] tensor_zero const &visitable) override {
    m_out << "0";
  }

  void operator()(tensor_pow const &visitable) override {
    m_out << "{";
    apply(visitable.expr_lhs(), Precedence::None);
    m_out << "}^{";
    apply(visitable.expr_rhs(), Precedence::None);
    m_out << "}";
  }

  template <class T> void operator()([[maybe_unused]] T const &visitable) {
    static_assert(sizeof(T) == 0,
                  "tensor_latex_printer: missing overload for this node type");
  }

private:
  auto apply(expression_holder<scalar_expression> const &expr,
             Precedence parent_precedence = Precedence::None) {
    scalar_latex_printer<StreamType> printer(m_out, m_config);
    printer.apply(expr, parent_precedence);
  }

  void apply(expression_holder<tensor_to_scalar_expression> const &expr,
             Precedence parent_precedence = Precedence::None);

  using base::m_config;
  using base::m_out;
  Precedence m_parent_precedence;
};

} // namespace numsim::cas

#endif // TENSOR_LATEX_PRINTER_H
