#include <algorithm>
#include <ranges>
#include <sstream>
#include <vector>

#include <numsim_cas/core/print_mul_fractions.h>
#include <numsim_cas/scalar/scalar_domain_traits.h>
#include <numsim_cas/scalar/visitors/scalar_latex_printer.h>
#include <numsim_cas/scalar/visitors/scalar_printer.h>

namespace numsim::cas {

template <typename Stream>
void scalar_latex_printer<Stream>::apply(
    expression_holder<scalar_expression> const &expr,
    Precedence parent_precedence) {
  if (expr.is_valid()) {
    m_parent_precedence = parent_precedence;
    static_cast<const scalar_visitable_t &>(expr.get())
        .accept(static_cast<base_visitor &>(*this));
    m_first_term = false;
  }
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(scalar const &visitable) {
  m_out << visitable.name();
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(
    scalar_named_expression const &visitable) {
  m_out << visitable.name();
  if (m_first_term) {
    m_out << " = ";
    apply(visitable.expr());
  }
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(
    scalar_constant const &visitable) {
  if (auto *r = std::get_if<rational_t>(&visitable.value().raw())) {
    m_out << "\\frac{" << r->num << "}{" << r->den << "}";
  } else {
    m_out << visitable.value();
  }
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(
    [[maybe_unused]] scalar_one const &visitable) {
  m_out << "1";
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(
    [[maybe_unused]] scalar_zero const &visitable) {
  m_out << "0";
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(scalar_mul const &visitable) {
  using traits = domain_traits<scalar_expression>;
  const auto parent_precedence{m_parent_precedence};

  auto [num, denom] = partition_mul_fractions<traits>(visitable.symbol_map());

  std::map<expr_t, expr_t, detail::scalar_pretty_printer> sorted_map;
  for (auto &child : num) {
    sorted_map[child] = child;
  }

  if (!denom.empty()) {
    // \frac{numerator}{denominator}
    m_out << "\\frac{";

    bool first = true;
    if (visitable.coeff().is_valid()) {
      apply(visitable.coeff(), Precedence::None);
      first = false;
    }
    for (auto &child : sorted_map | std::views::values) {
      if (!first)
        m_out << " \\cdot ";
      apply(child, Precedence::None);
      first = false;
    }
    if (first) {
      // numerator is empty (all went to denom), write "1"
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
    for (auto &child : sorted_map | std::views::values) {
      if (!first)
        m_out << " \\cdot ";
      apply(child, precedence);
      first = false;
    }

    end(precedence, parent_precedence);
  }
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(scalar_add const &visitable) {
  constexpr auto precedence{Precedence::Addition};
  const auto parent_precedence{m_parent_precedence};
  begin(precedence, parent_precedence);
  const auto values{visitable.symbol_map_values()};
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

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(
    scalar_negative const &visitable) {
  constexpr auto precedence{Precedence::Negative};
  const auto parent_precedence{m_parent_precedence};

  m_out << "-";
  begin(precedence, parent_precedence);
  apply(visitable.expr(), precedence);
  end(precedence, parent_precedence);
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(scalar_log const &visitable) {
  print_unary("\\ln", visitable);
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(scalar_sqrt const &visitable) {
  m_out << "\\sqrt{";
  apply(visitable.expr());
  m_out << "}";
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(scalar_exp const &visitable) {
  print_unary("\\exp", visitable);
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(scalar_sign const &visitable) {
  print_unary("\\operatorname{sgn}", visitable);
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(scalar_abs const &visitable) {
  m_out << "\\left|";
  apply(visitable.expr());
  m_out << "\\right|";
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(scalar_pow const &visitable) {
  const auto &lhs{visitable.expr_lhs()};
  const auto &rhs{visitable.expr_rhs()};
  m_out << "{";
  apply(lhs, Precedence::Unary);
  m_out << "}^{";
  apply(rhs);
  m_out << "}";
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(scalar_tan const &visitable) {
  print_unary("\\tan", visitable);
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(scalar_sin const &visitable) {
  print_unary("\\sin", visitable);
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(scalar_cos const &visitable) {
  print_unary("\\cos", visitable);
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(scalar_atan const &visitable) {
  print_unary("\\arctan", visitable);
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(scalar_asin const &visitable) {
  print_unary("\\arcsin", visitable);
}

template <typename Stream>
void scalar_latex_printer<Stream>::operator()(scalar_acos const &visitable) {
  print_unary("\\arccos", visitable);
}

template class scalar_latex_printer<std::ostream>;
template class scalar_latex_printer<std::stringstream>;

} // namespace numsim::cas
