#include <algorithm>
#include <numsim_cas/scalar/visitors/scalar_printer.h>
#include <ranges>
#include <vector>

namespace numsim::cas {

template <typename Stream>
void scalar_printer<Stream>::apply(
    expression_holder<scalar_expression> const &expr,
    [[maybe_unused]] Precedence parent_precedence) noexcept {
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
                   [[maybe_unused]] Precedence parent_precedence*/) noexcept {
  m_out << visitable.name();
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_function const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) noexcept {
  m_out << visitable.name();
  if (m_first_term) {
    m_out << " = ";
    apply(visitable.expr());
  }
}

template <typename Stream>
void scalar_printer<Stream>::operator()(
    [[maybe_unused]] scalar_constant const &visitable) noexcept {
  m_out << visitable.value();
}

template <typename Stream>
void scalar_printer<Stream>::operator()(
    [[maybe_unused]] scalar_one const &visitable) noexcept {
  m_out << "1";
}

template <typename Stream>
void scalar_printer<Stream>::operator()(
    [[maybe_unused]] scalar_zero const &visitable) noexcept {
  m_out << "0";
}

template <typename Stream>
void scalar_printer<Stream>::operator()(scalar_mul const &visitable) noexcept {
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
      if (auto expr_const{is_same_r<scalar_constant>(expr_rhs)}) {
        if (expr_const->get().value() < 0) {
          power.push_back(expr);
          numerator.push_back(power_expr->get().expr_lhs());
          denominator.push_back(
              make_expression<scalar_constant>(-expr_const->get().value()));
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
        if ((*number) != 1) {
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

template <typename Stream>
void scalar_printer<Stream>::operator()(scalar_add const &visitable) noexcept {
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
// void scalar_printer<Stream>::operator()(scalar_div const &visitable/*,
//                 Precedence parent_precedence*/) noexcept {
//   constexpr auto precedence{Precedence::Multiplication};
//   const auto parent_precedence{m_parent_precedence};

//   begin(precedence, parent_precedence);
//   apply(visitable.expr_lhs(), Precedence::Division_LHS);
//   m_out << "/";
//   apply(visitable.expr_rhs(), Precedence::Division_RHS);
//   end(precedence, parent_precedence);
// }

template <typename Stream>
void scalar_printer<Stream>::operator()(
    scalar_rational const &visitable) noexcept {
  constexpr auto precedence{Precedence::Multiplication};
  const auto parent_precedence{m_parent_precedence};

  begin(precedence, parent_precedence);
  apply(visitable.expr_lhs(), Precedence::Division_LHS);
  m_out << "/";
  apply(visitable.expr_rhs(), Precedence::Division_RHS);
  end(precedence, parent_precedence);
}

template <typename Stream>
void scalar_printer<Stream>::operator()(scalar_negative const
                                            &visitable /*,
Precedence parent_precedence*/) noexcept {
  constexpr auto precedence{Precedence::Negative};
  const auto parent_precedence{m_parent_precedence};

  m_out << "-";
  begin(precedence, parent_precedence);
  apply(visitable.expr(), precedence);
  end(precedence, parent_precedence);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_log const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) noexcept {
  print_unary("log", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_sqrt const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) noexcept {
  print_unary("sqrt", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_exp const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) noexcept {
  print_unary("exp", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_sign const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) noexcept {
  print_unary("sign", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_abs const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) noexcept {
  print_unary("abs", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_pow const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) noexcept {
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
                   [[maybe_unused]] Precedence parent_precedence*/) noexcept {
  print_unary("tan", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_sin const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) noexcept {
  print_unary("sin", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_cos const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) noexcept {
  print_unary("cos", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_atan const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) noexcept {
  print_unary("atan", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_asin const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) noexcept {
  print_unary("asin", visitable);
}

template<typename Stream>
  void scalar_printer<Stream>::operator()(scalar_acos const &visitable/*,
                   [[maybe_unused]] Precedence parent_precedence*/) noexcept {
  print_unary("acos", visitable);
}

template class scalar_printer<std::ostream>;

} // namespace numsim::cas
