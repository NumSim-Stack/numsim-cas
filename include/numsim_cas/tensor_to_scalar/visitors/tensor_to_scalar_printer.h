#ifndef TENSOR_TO_SCALAR_PRINTER_H
#define TENSOR_TO_SCALAR_PRINTER_H

#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_expression.h>

#include <numsim_cas/printer_base.h>
#include <numsim_cas/tensor/sequence.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_add.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar/tensor_to_scalar_mul.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_add.h>
#include <numsim_cas/tensor_to_scalar/operators/tensor_to_scalar_with_scalar/tensor_to_scalar_with_scalar_mul.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_definitions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_io.h>

namespace numsim::cas {

template <typename StreamType>
class tensor_to_scalar_printer final
    : public tensor_to_scalar_visitor_const_t,
      public printer_base<tensor_to_scalar_printer<StreamType>, StreamType> {
public:
  using base_visitor =
      printer_base<tensor_to_scalar_printer<StreamType>, StreamType>;
  friend base_visitor;
  using base_visitor::begin;
  using base_visitor::end;
  using base_visitor::print_unary;

  explicit tensor_to_scalar_printer(StreamType &out) : base_visitor(out) {}
  tensor_to_scalar_printer(tensor_to_scalar_printer const &) = delete;
  tensor_to_scalar_printer(tensor_to_scalar_printer &&) = delete;
  const tensor_to_scalar_printer &
  operator=(tensor_to_scalar_printer const &) = delete;

  auto apply(expression_holder<tensor_to_scalar_expression> const &expr,
             [[maybe_unused]] Precedence parent_precedence =
                 Precedence::None) noexcept {
    if (expr.is_valid()) {
      m_parent_precedence = parent_precedence;
      static_cast<const tensor_to_scalar_visitable_t &>(expr.get())
          .accept(static_cast<tensor_to_scalar_visitor_const_t &>(*this));
    }
  }

  void operator()(tensor_trace const &visitable) noexcept {
    print_unary("tr", visitable);
  }

  void operator()(tensor_dot const &visitable) noexcept {
    print_unary("dot", visitable);
  }

  void operator()(tensor_norm const &visitable) noexcept {
    print_unary("norm", visitable);
  }

  void operator()(tensor_det const &visitable) noexcept {
    print_unary("det", visitable);
  }

  void operator()(tensor_to_scalar_negative const &visitable) noexcept {
    constexpr auto precedence{Precedence::Unary};
    m_out << "-";
    // begin(precedence, m_parent_precedence);
    apply(visitable.expr(), precedence);
    // end(precedence, m_parent_precedence);
  }

  void operator()(tensor_to_scalar_mul const &visitable) noexcept {
    constexpr auto precedence{Precedence::Multiplication};
    begin(precedence, m_parent_precedence);
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
    end(precedence, m_parent_precedence);
  }

  void operator()(tensor_to_scalar_log const &visitable) noexcept {
    print_unary("log", visitable);
  }

  void operator()(tensor_to_scalar_add const &visitable) noexcept {
    constexpr auto precedence{Precedence::Addition};
    begin(precedence, m_parent_precedence);

    bool first{false};
    if (visitable.coeff().is_valid()) {
      apply(visitable.coeff(), precedence);
      first = true;
    }
    for (auto &child : visitable.hash_map() | std::views::values) {
      if (first && !is_same<tensor_to_scalar_negative>(child)) {
        m_out << "+";
      }
      apply(child, precedence);
      first = true;
    }

    end(precedence, m_parent_precedence);
  }

  // void operator()(tensor_to_scalar_div const &visitable) {
  //   constexpr auto precedence{Precedence::Multiplication};
  //   begin(precedence, m_parent_precedence);
  //   apply(visitable.expr_lhs(), Precedence::Division_LHS);
  //   m_out << "/";
  //   apply(visitable.expr_rhs(), Precedence::Division_RHS);
  //   end(precedence, m_parent_precedence);
  // }

  void operator()(tensor_to_scalar_pow const &visitable) noexcept {
    m_out << "pow(";
    m_out << visitable.expr_lhs();
    m_out << ",";
    m_out << visitable.expr_rhs();
    m_out << ")";
  }

  // void operator()(tensor_to_scalar_pow_with_scalar_exponent const &visitable)
  // noexcept{
  //   m_out << "pow(";
  //   m_out << visitable.expr_lhs();
  //   m_out << ",";
  //   m_out << visitable.expr_rhs();
  //   m_out << ")";
  // }

  // void operator()(tensor_to_scalar_with_scalar_mul const &visitable) {
  //   constexpr auto precedence{Precedence::Multiplication};
  //   begin(precedence, m_parent_precedence);
  //   print(m_out, visitable.expr_lhs(), precedence);
  //   m_out << "*";
  //   apply(visitable.expr_rhs(), precedence);
  //   end(precedence, m_parent_precedence);
  // }

  // void operator()(tensor_to_scalar_with_scalar_add const &visitable) {
  //   constexpr auto precedence{Precedence::Addition};
  //   begin(precedence, m_parent_precedence);
  //   print(m_out, visitable.expr_lhs(), precedence);
  //   m_out << "+";
  //   apply(visitable.expr_rhs(), precedence);
  //   end(precedence, m_parent_precedence);
  // }

  // void operator()(tensor_to_scalar_with_scalar_div const &visitable) {
  //   constexpr auto precedence{Precedence::Multiplication};
  //   begin(precedence, m_parent_precedence);
  //   apply(visitable.expr_lhs(), Precedence::Division_LHS);
  //   m_out << "/";
  //   print(m_out, visitable.expr_rhs(), Precedence::Division_RHS);
  //   end(precedence, m_parent_precedence);
  // }

  // void operator()(scalar_with_tensor_to_scalar_div const &visitable) {
  //   constexpr auto precedence{Precedence::Multiplication};
  //   begin(precedence, m_parent_precedence);
  //   print(m_out, visitable.expr_lhs(), Precedence::Division_LHS);
  //   m_out << "/";
  //   apply(visitable.expr_rhs(), Precedence::Division_RHS);
  //   end(precedence, m_parent_precedence);
  // }

  void operator()(tensor_inner_product_to_scalar const &visitable) noexcept {
    const auto &indices_lhs{visitable.indices_lhs()};
    const auto &indices_rhs{visitable.indices_rhs()};

    if (indices_lhs == sequence{1, 2} && indices_rhs == sequence{1, 2}) {
      begin(Precedence::Multiplication, m_parent_precedence);
      apply(visitable.expr_lhs(), Precedence::Multiplication);
      m_out << ":";
      apply(visitable.expr_rhs(), Precedence::Multiplication);
      end(Precedence::Multiplication, m_parent_precedence);
    } else if (indices_lhs == sequence{1, 2, 3, 4} &&
               indices_rhs == sequence{1, 2, 3, 4}) {
      begin(Precedence::Multiplication, m_parent_precedence);
      apply(visitable.expr_lhs(), Precedence::Multiplication);
      m_out << "::";
      apply(visitable.expr_rhs(), Precedence::Multiplication);
      end(Precedence::Multiplication, m_parent_precedence);
    } else {
      m_out << "dot(";
      apply(visitable.expr_lhs(), Precedence::None);
      m_out << ", ";
      m_out << indices_lhs;
      m_out << ", ";
      apply(visitable.expr_rhs(), Precedence::None);
      m_out << ", ";
      m_out << indices_rhs;
      m_out << ")";
    }
  }

  /**
   * @brief Tensor to scalar one
   *
   * @note Will be set to zero in apply function
   */
  void
  operator()([[maybe_unused]] tensor_to_scalar_one const &visitable) noexcept {
    m_out << "1";
  }

  /**
   * @brief Tensor to scalar zero
   *
   * @note Will be set to zero in apply function
   */
  void
  operator()([[maybe_unused]] tensor_to_scalar_zero const &visitable) noexcept {
    m_out << "0";
  }

  /**
   * @brief Scalar expression converted to tensor_to_scalar
   *
   */
  void operator()([[maybe_unused]] tensor_to_scalar_scalar_wrapper const
                      &visitable) noexcept {
    apply(visitable.expr(), m_parent_precedence);
  }

  /**
   * @brief Default overload for safty reasons.
   */
  template <class T>
  void operator()([[maybe_unused]] T const &visitable) noexcept {
    static_assert(sizeof(T) == 0, "tensor_to_scalar_printer: missing "
                                  "overload for this node type");
  }

private:
  void apply(expression_holder<tensor_expression> const &expr,
             Precedence parent_precedence = Precedence::None) noexcept;

  void apply(expression_holder<scalar_expression> const &expr,
             Precedence parent_precedence = Precedence::None) noexcept;

  using base_visitor::m_out; ///< The output stream used for printing.
  Precedence m_parent_precedence;
};

} // namespace numsim::cas
#endif // TENSOR_TO_SCALAR_PRINTER_H
