#include <algorithm>
#include <ranges>
#include <sstream>
#include <vector>

#include <numsim_cas/core/print_mul_fractions.h>
#include <numsim_cas/scalar/scalar_domain_traits.h>
#include <numsim_cas/scalar/visitors/scalar_printer.h>

namespace numsim::cas {

template <typename Stream>
void scalar_printer<Stream>::apply(
    expression_holder<scalar_expression> const &expr,
    [[maybe_unused]] Precedence parent_precedence) {
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

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) {
  m_out << visitable.name();
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_named_expression const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) {
  m_out << visitable.name();
  if (m_first_term) {
    m_out << " = ";
    apply(visitable.expr());
  }
}

template <typename Stream>
void scalar_printer<Stream>::operator()(
    [[maybe_unused]] scalar_constant const &visitable) {
  if (auto *r = std::get_if<rational_t>(&visitable.value().raw())) {
    constexpr auto precedence{Precedence::Division_LHS};
    const auto parent_precedence{m_parent_precedence};
    begin(precedence, parent_precedence);
    m_out << r->num << "/" << r->den;
    end(precedence, parent_precedence);
  } else {
    m_out << visitable.value();
  }
}

template <typename Stream>
void scalar_printer<Stream>::operator()(
    [[maybe_unused]] scalar_one const &visitable) {
  m_out << "1";
}

template <typename Stream>
void scalar_printer<Stream>::operator()(
    [[maybe_unused]] scalar_zero const &visitable) {
  m_out << "0";
}

template <typename Stream>
void scalar_printer<Stream>::operator()(scalar_mul const &visitable) {
  using traits = domain_traits<scalar_expression>;
  constexpr auto precedence{Precedence::Multiplication};
  const auto parent_precedence{m_parent_precedence};

  begin(precedence, parent_precedence);

  auto [num, denom] = partition_mul_fractions<traits>(visitable.symbol_map());

  std::map<expr_t, expr_t, detail::scalar_pretty_printer> sorted_map;
  for (auto &child : num) {
    sorted_map[child] = child;
  }

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

  if (!denom.empty()) {
    m_out << "/";
    for (auto &entry : denom) {
      if (entry.pos_exponent != scalar_number{1}) {
        m_out << "pow(";
        apply(entry.base);
        m_out << ",";
        m_out << entry.pos_exponent;
        m_out << ")";
      } else {
        apply(entry.base, Precedence::Division_RHS);
      }
    }
  }
  end(precedence, parent_precedence);
}

template <typename Stream>
void scalar_printer<Stream>::operator()(scalar_add const &visitable) {
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
void scalar_printer<Stream>::operator()(scalar_negative const
                                            &visitable /*,
Precedence parent_precedence*/) {
  constexpr auto precedence{Precedence::Negative};
  const auto parent_precedence{m_parent_precedence};

  m_out << "-";
  begin(precedence, parent_precedence);
  apply(visitable.expr(), precedence);
  end(precedence, parent_precedence);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_log const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) {
  print_unary("log", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_sqrt const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) {
  print_unary("sqrt", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_exp const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) {
  print_unary("exp", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_sign const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) {
  print_unary("sign", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_abs const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) {
  print_unary("abs", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_pow const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) {
  const auto &lhs{visitable.expr_lhs()};
  const auto &rhs{visitable.expr_rhs()};
  m_out << "pow(";
  apply(lhs);
  m_out << ",";
  apply(rhs);
  m_out << ")";
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_tan const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) {
  print_unary("tan", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_sin const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) {
  print_unary("sin", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_cos const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) {
  print_unary("cos", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_atan const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) {
  print_unary("atan", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_asin const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) {
  print_unary("asin", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_acos const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) {
  print_unary("acos", visitable);
}

template class scalar_printer<std::ostream>;
template class scalar_printer<std::stringstream>;

} // namespace numsim::cas
