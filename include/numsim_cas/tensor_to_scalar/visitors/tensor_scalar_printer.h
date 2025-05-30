#ifndef TENSOR_SCALAR_PRINTER_H
#define TENSOR_SCALAR_PRINTER_H

#include <string>
#include <algorithm>
#include <functional>
#include <iostream>
#include <variant>
#include <vector>
#include "../../printer_base.h"
#include "../../numsim_cas_type_traits.h"
#include "../../scalar/visitors/scalar_printer.h"

namespace numsim::cas {


template <typename ValueType, typename StreamType>
class tensor_to_scalar_printer final : public printer_base<tensor_to_scalar_printer<ValueType, StreamType>, StreamType> {
public:
  using base = printer_base<tensor_to_scalar_printer<ValueType, StreamType>, StreamType>;
  using base::begin;
  using base::end;
  using base::print_unary;

  explicit tensor_to_scalar_printer(StreamType &out) : base(out) {}
  tensor_to_scalar_printer(tensor_to_scalar_printer const &) = delete;
  tensor_to_scalar_printer(tensor_to_scalar_printer &&) = delete;
  const tensor_to_scalar_printer &operator=(tensor_to_scalar_printer const &) = delete;

  auto apply(expression_holder<tensor_to_scalar_expression<ValueType>> const&expr,
             Precedence parent_precedence = Precedence::None) {
    if (expr.is_valid()) {
      std::visit([this, parent_precedence](
                     auto &&arg) { (*this)(arg, parent_precedence); },
                 *expr);
    }
  }

  auto apply(expression_holder<tensor_expression<ValueType>> const&expr,
             [[maybe_unused]]Precedence parent_precedence = Precedence::None) {
    m_out << expr;
  }

  auto apply(expression_holder<scalar_expression<ValueType>> const&expr,
             [[maybe_unused]]Precedence parent_precedence = Precedence::None) {
    scalar_printer<ValueType, StreamType> printer(m_out);
    printer.apply(expr, parent_precedence);
  }

  void operator()(tensor_trace<ValueType> const&visitable, [[maybe_unused]] Precedence parent_precedence) {
    print_unary("tr", visitable);
  }

  void operator()(tensor_dot<ValueType> const&visitable, [[maybe_unused]] Precedence parent_precedence) {
    print_unary("dot", visitable);
  }

  void operator()(tensor_norm<ValueType> const&visitable, [[maybe_unused]] Precedence parent_precedence) {
    print_unary("norm", visitable);
  }

  void operator()(tensor_det<ValueType> const&visitable, [[maybe_unused]] Precedence parent_precedence) {
    print_unary("det", visitable);
  }

  void operator()(tensor_to_scalar_negative<ValueType> const &visitable,
                  Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Unary};
    m_out << "-";
    begin(precedence, parent_precedence);
    apply(visitable.expr(), precedence);
    end(precedence, parent_precedence);
  }

  void operator()(tensor_to_scalar_mul<ValueType> const &visitable,
                  [[maybe_unused]]Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Multiplication};
    //begin(precedence, parent_precedence);
    bool first = true;
    if (visitable.coeff().is_valid()) {
      apply(visitable.coeff(), precedence);
      m_out << "*";
    }
    for (auto &child : visitable.hash_map() | std::views::values) {
      if (!first)
        m_out << "*";
      apply(child, precedence);
      first = false;
    }
    //end(precedence, parent_precedence);
  }

  void operator()(tensor_to_scalar_add<ValueType> const &visitable,
                  Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Addition};
    begin(precedence, parent_precedence);

    bool first = true;
    if (visitable.coeff().is_valid()) {
      apply(visitable.coeff(), precedence);
      m_out << "+";
    }
    for (auto &child : visitable.hash_map() | std::views::values) {
      if (!first && !is_same<tensor_to_scalar_negative<ValueType>>(child)){
        m_out << "+";
      }
      apply(child, precedence);
      first = false;
    }

    end(precedence, parent_precedence);
  }

  void operator()(tensor_to_scalar_sub<ValueType> const &visitable,
                  Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Addition};
    begin(precedence, parent_precedence);

    bool first = true;
    if (visitable.coeff().is_valid()) {
      apply(visitable.coeff(), precedence);
      m_out << "*";
    }

    if (visitable.size() > 1) {
      m_out << "(";
    }

    for (auto &child : visitable.hash_map() | std::views::values) {
      if (!first)
        m_out << "-";
      apply(child, precedence);
      first = false;
    }

    if (visitable.size() > 1) {
      m_out << ")";
    }

    end(precedence, parent_precedence);
  }

  void operator()(tensor_to_scalar_div<ValueType> const &visitable,
                  Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Multiplication};
    begin(precedence, parent_precedence);
    apply(visitable.expr_lhs(), precedence);
    m_out << "/";
    apply(visitable.expr_rhs(), precedence);
    end(precedence, parent_precedence);
  }

  void operator()(tensor_to_scalar_pow<ValueType> const &visitable,
                  [[maybe_unused]]Precedence parent_precedence) {
    m_out<<"pow(";
    m_out<<visitable.expr_lhs();
    m_out<<",";
    m_out<<visitable.expr_rhs();
    m_out<<")";
  }

  void operator()(tensor_to_scalar_pow_with_scalar_exponent<ValueType> const &visitable,
                  [[maybe_unused]]Precedence parent_precedence) {
    m_out<<"pow(";
    m_out<<visitable.expr_lhs();
    m_out<<",";
    m_out<<visitable.expr_rhs();
    m_out<<")";
  }

  void operator()(tensor_to_scalar_with_scalar_mul<ValueType> const &visitable,
                  Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Multiplication};
    begin(precedence, parent_precedence);
    apply(visitable.expr_lhs(), precedence);
    m_out<<"*";
    apply(visitable.expr_rhs(), precedence);
    end(precedence, parent_precedence);
  }

  void operator()(tensor_to_scalar_with_scalar_add<ValueType> const &visitable,
                  Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Addition};
    begin(precedence, parent_precedence);
    apply(visitable.expr_lhs(), precedence);
    m_out<<"+";
    apply(visitable.expr_rhs(), precedence);
    end(precedence, parent_precedence);
  }

  void operator()(tensor_to_scalar_with_scalar_div<ValueType> const &visitable,
                  Precedence parent_precedence) {
    begin(Precedence::Division, parent_precedence);
    apply(visitable.expr_lhs(), Precedence::Division);
    end(Precedence::Division, parent_precedence);
    m_out<<"/";
    apply(visitable.expr_rhs(), Precedence::Division);
    end(Precedence::Division, parent_precedence);
  }

  void operator()(scalar_with_tensor_to_scalar_div<ValueType> const &visitable,
                  Precedence parent_precedence) {
    begin(Precedence::Division, parent_precedence);
    apply(visitable.expr_lhs(), Precedence::Division);
    m_out<<"/";
    apply(visitable.expr_rhs(), Precedence::Division);
    end(Precedence::Division, parent_precedence);
  }

  template<typename Expr>
  void operator()([[maybe_unused]]Expr const &visitable,
                  [[maybe_unused]]Precedence parent_precedence) {
    assert(0);
  }

private:
  using base::m_out; ///< The output stream used for printing.
};


}
#endif // TENSOR_SCALAR_PRINTER_H
