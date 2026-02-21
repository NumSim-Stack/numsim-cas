#ifndef TENSOR_PRINTER_H
#define TENSOR_PRINTER_H

#include <numsim_cas/printer_base.h>
#include <numsim_cas/scalar/scalar_functions.h>
#include <numsim_cas/scalar/visitors/scalar_printer.h>
#include <numsim_cas/tensor/tensor_definitions.h>
#include <numsim_cas/tensor/tensor_expression.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_io.h>

#include <algorithm>
#include <string>
#include <vector>

namespace numsim::cas {

/**
 * @brief Class for printing scalar expressions with correct precedence and
 * formatting.
 *
 * @tparam ValueType The type of the scalar values.
 * @tparam StreamType The type of the output stream.
 */
template <typename StreamType>
class tensor_printer final
    : public tensor_visitor_const_t,
      public printer_base<tensor_printer<StreamType>, StreamType> {
public:
  using base = printer_base<tensor_printer<StreamType>, StreamType>;
  using expr_t = expression_holder<tensor_expression>;
  using base_visitor = tensor_visitor_const_t;
  using base::begin;
  using base::end;
  using base::print_unary;

  /**
   * @brief Constructor for tensor_printer.
   *
   * @param out The output stream to which expressions will be printed.
   */
  explicit tensor_printer(StreamType &out) : base(out) {}

  // Disable copy and move operations
  tensor_printer(tensor_printer const &) = delete;
  tensor_printer(tensor_printer &&) = delete;
  const tensor_printer &operator=(tensor_printer const &) = delete;

  /**
   * @brief Applies the printer to an expression.
   *
   * @param expr The tensor expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  auto apply(expression_holder<tensor_expression> const &expr,
             [[maybe_unused]] Precedence parent_precedence = Precedence::None) {
    if (expr.is_valid()) {
      m_parent_precedence = parent_precedence;
      static_cast<const tensor_visitable_t &>(expr.get())
          .accept(static_cast<base_visitor &>(*this));
      // m_first_term = false;
    }
  }

  /**
   * @brief Prints a tensor value.
   *
   * @param visitable The tensor value to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(tensor const &visitable) { m_out << visitable.name(); }

  /**
   * @brief Prints a identity tensor.
   *
   * @param visitable The idenity tensor to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()([[maybe_unused]] identity_tensor const &visitable) {
    m_out << "I{" << visitable.rank() << "}";
  }

  /**
   * @brief Prints a projection tensor.
   *
   * @param visitable The projection tensor to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()([[maybe_unused]] tensor_projector const &visitable) {
    tensor_trace_print_visitor perm_trace_printer(visitable);
    auto to_print{perm_trace_printer.apply()};
    m_out << to_print << "{" << visitable.rank() << "}";
  }

  /**
   * @brief Prints a tensor addition expression.
   *
   * @param visitable The tensor addition expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(tensor_add const &visitable) {
    constexpr auto precedence{Precedence::Addition};
    const auto parent_precedence{m_parent_precedence};

    begin(precedence, parent_precedence);
    const auto values{visitable.hash_map() | std::views::values};
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

  void operator()(tensor_mul const &visitable) {
    constexpr auto precedence{Precedence::Multiplication};
    const auto parent_precedence{m_parent_precedence};

    begin(precedence, parent_precedence);
    bool first = true;
    if (visitable.coeff().is_valid()) {
      apply(visitable.coeff(), precedence);
      m_out << "*";
    }
    for (auto &child : visitable.data()) {
      if (!first)
        m_out << "*";
      apply(child, precedence);
      first = false;
    }
    end(precedence, parent_precedence);
  }

  /**
   * @brief Prints a tensor negative expression.
   *
   * @param visitable The tensor negative expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(tensor_negative const &visitable) {
    constexpr auto precedence{Precedence::Negative};
    const auto parent_precedence{m_parent_precedence};

    m_out << "-";
    begin(precedence, parent_precedence);
    apply(visitable.expr(), precedence);
    end(precedence, parent_precedence);
  }

  /**
   * @brief Prints an inner product.
   *
   * @param visitable The inner product expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(inner_product_wrapper const &visitable) {
    constexpr auto precedence{Precedence::Multiplication};

    const auto &indices_lhs{visitable.indices_lhs()};
    const auto &indices_rhs{visitable.indices_rhs()};

    // Projector function notation: P:A → dev(A), sym(A), vol(A), skew(A)
    if (indices_lhs == sequence{3, 4} && indices_rhs == sequence{1, 2} &&
        is_same<tensor_projector>(visitable.expr_lhs())) {
      auto const &proj = visitable.expr_lhs().template get<tensor_projector>();
      if (proj.acts_on_rank() == 2) {
        auto const &sp = proj.space();
        const char *fn = nullptr;
        if (std::holds_alternative<Symmetric>(sp.perm) &&
            std::holds_alternative<DeviatoricTag>(sp.trace))
          fn = "dev";
        else if (std::holds_alternative<Symmetric>(sp.perm) &&
                 std::holds_alternative<AnyTraceTag>(sp.trace))
          fn = "sym";
        else if (std::holds_alternative<Symmetric>(sp.perm) &&
                 std::holds_alternative<VolumetricTag>(sp.trace))
          fn = "vol";
        else if (std::holds_alternative<Skew>(sp.perm) &&
                 std::holds_alternative<AnyTraceTag>(sp.trace))
          fn = "skew";
        if (fn) {
          m_out << fn << "(";
          apply(visitable.expr_rhs(), Precedence::None);
          m_out << ")";
          return;
        }
      }
    }

    // single contraction
    const auto rank_lhs{call_tensor::rank(visitable.expr_lhs())};
    if (indices_lhs == sequence{rank_lhs} && indices_rhs == sequence{1}) {
      apply(visitable.expr_lhs(), precedence);
      m_out << "*";
      apply(visitable.expr_rhs(), precedence);
      return;
    }

    // double contraction
    if (indices_lhs == sequence{rank_lhs - 1, rank_lhs} &&
        indices_rhs == sequence{1, 2}) {
      apply(visitable.expr_lhs(), precedence);
      m_out << ":";
      apply(visitable.expr_rhs(), precedence);
      return;
    }

    // fourth contraction
    if (indices_lhs ==
            sequence{rank_lhs - 3, rank_lhs - 2, rank_lhs - 1, rank_lhs} &&
        indices_rhs == sequence{1, 2, 3, 4}) {
      apply(visitable.expr_lhs(), precedence);
      m_out << "::";
      apply(visitable.expr_rhs(), precedence);
      return;
    }

    m_out << "inner(";
    apply(visitable.expr_lhs(), precedence);
    m_out << indices_lhs;
    m_out << ", ";
    apply(visitable.expr_rhs(), precedence);
    m_out << ", ";
    m_out << indices_rhs;
    m_out << ")";
  }

  /**
   * @brief Prints a basis change expression.
   *
   * @param visitable The basis change expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(basis_change_imp const &visitable) {
    auto indices_temp{visitable.indices()};
    if (indices_temp == sequence{2, 1}) {
      m_out << "trans(";
      apply(visitable.expr(), m_parent_precedence);
      m_out << ")";
    } else {
      m_out << "permute_indices(";
      apply(visitable.expr(), m_parent_precedence);
      m_out << ", ";
      m_out << visitable.indices();
      m_out << ")";
    }
  }

  /**
   * @brief Prints an outer product.
   *
   * @param visitable The outer product expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(outer_product_wrapper const &visitable) {
    auto indices_temp_lhs{visitable.indices_lhs()};
    auto indices_temp_rhs{visitable.indices_rhs()};

    if (indices_temp_lhs == sequence{1, 4} &&
        indices_temp_rhs == sequence{2, 3}) {
      m_out << "otimesl(";
      apply(visitable.expr_lhs(), Precedence::None);
      m_out << ",";
      apply(visitable.expr_rhs(), Precedence::None);
      m_out << ")";
    } else if (indices_temp_lhs == sequence{1, 3} &&
               indices_temp_rhs == sequence{2, 4}) {
      m_out << "otimesu(";
      apply(visitable.expr_lhs(), Precedence::None);
      m_out << ",";
      apply(visitable.expr_rhs(), Precedence::None);
      m_out << ")";
    } else {
      m_out << "outer(";
      apply(visitable.expr_lhs(), Precedence::None);
      m_out << ", ";
      m_out << indices_temp_lhs;
      // m_out << ", [";
      // base::print_sequence(m_out, indices_temp_lhs, ',');
      // m_out << "]";
      m_out << ", ";
      apply(visitable.expr_rhs(), Precedence::None);
      m_out << ", ";
      m_out << indices_temp_rhs;
      // m_out << ", [";
      // base::print_sequence(m_out, indices_temp_rhs, ',');
      // m_out << "]";
      m_out << ")";
    }
  }

  void operator()([[maybe_unused]] simple_outer_product const &visitable) {
    constexpr auto precedence{Precedence::Multiplication};
    const auto parent_precedence{m_parent_precedence};

    begin(precedence, parent_precedence);
    bool first = true;
    if (visitable.coeff().is_valid()) {
      apply(visitable.coeff(), precedence);
      m_out << "*";
    }
    m_out << "outer(";
    for (auto &child : visitable.data()) {
      if (!first)
        m_out << ",";
      apply(child, precedence);
      first = false;
    }
    m_out << ")";
    end(precedence, parent_precedence);
  }

  /**
   * @brief Prints a Kronecker delta expression.
   *
   * @param visitable The Kronecker delta expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()([[maybe_unused]] kronecker_delta const &visitable) {
    m_out << "I";
  }

  //  /**
  //   * @brief Prints a simple outer product expression.
  //   *
  //   * @param visitable The simple outer product expression to be printed.
  //   * @param parent_precedence The precedence of the parent expression.
  //   */
  //  void operator()(simple_outer_product &visitable, Precedence
  //  parent_precedence) {
  //    //constexpr auto precedence{Precedence::Multiplication};
  //    m_out << "simple_outer(";
  //    //bool first = true;
  ////    for (auto &child : visitable) {
  ////      if (!first) {
  ////        m_out << ", ";
  ////      }
  ////      apply(child, precedence);
  ////      first = false;
  ////    }
  //    m_out << ")";
  //  }

  /**
   * @brief Prints a tensor scalar multiplication expression.
   *
   * @param visitable The tensor scalar multiplication expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(tensor_scalar_mul const &visitable) {
    constexpr auto precedence{Precedence::Multiplication};
    const auto parent_precedence{m_parent_precedence};

    begin(precedence, parent_precedence);

    using scalar_expr_t = expression_holder<scalar_expression>;
    scalar_printer<StreamType> sp(m_out);

    // Special case: tensor * pow(base, -exp)  ->  tensor / pow(base, exp)
    if (auto pow_e = is_same_r<scalar_pow>(visitable.expr_lhs())) {
      const auto &p = pow_e->get();

      if (auto neg_e = is_same_r<scalar_negative>(p.expr_rhs())) {
        const auto &neg = neg_e->get();
        const scalar_expr_t &exp_pos = neg.expr();
        // positive exponent expression

        // numerator: tensor
        apply(visitable.expr_rhs(), precedence);
        m_out << "/";

        // denominator
        if (const auto number = get_scalar_number(exp_pos)) {
          if (*number == 1) {
            // pow(base,1) -> base
            sp.apply(p.expr_lhs(), Precedence::Division_RHS);
          } else {
            m_out << "pow(";
            sp.apply(p.expr_lhs());
            m_out << ",";
            m_out << *number;
            m_out << ")";
          }
        } else {
          m_out << "pow(";
          sp.apply(p.expr_lhs());
          m_out << ",";
          sp.apply(exp_pos);
          m_out << ")";
        }

        end(precedence, parent_precedence);
        return;
      }
    }

    // scalar
    apply(visitable.expr_lhs(), precedence);
    m_out << "*";
    // tensor
    apply(visitable.expr_rhs(), precedence);

    end(precedence, parent_precedence);
  }

  // void
  // operator()([[maybe_unused]] tensor_scalar_mul const &visitable) {
  //   constexpr auto precedence{Precedence::Multiplication};
  //   const auto parent_precedence{m_parent_precedence};

  //   begin(precedence, parent_precedence);
  //   if(auto expr_pow{is_same_r<scalar_pow>(visitable.expr_lhs())}){
  //     if(auto
  //     expr_neg{is_same_r<scalar_negative>(expr_pow->get().expr_rhs())}){
  //       apply(visitable.expr_rhs(), precedence);
  //       m_out<<"/";
  //       if (const auto number{get_scalar_number(expr_neg->get().expr())}) {
  //         if ((*number) != 1) {
  //           m_out<<"pow(";
  //           apply(expr_pow->get().expr_lhs());
  //           m_out<<", " << (*number) << ")";
  //         }
  //       }else{
  //         scalar_printer<StreamType> scalar_printer(m_out);
  //         scalar_printer.apply(expr_pow->get().expr_lhs(),
  //         Precedence::Division_RHS);
  //       }
  //       end(precedence, parent_precedence);
  //       return;
  //     }
  //   }

  //   scalar_printer<StreamType> scalar_printer(m_out);
  //   scalar_printer.apply(visitable.expr_lhs(), precedence);
  //   m_out << "*";
  //   apply(visitable.expr_rhs(), precedence);
  //   end(precedence, parent_precedence);
  // }

  /**
   * @brief Prints a tensor scalar division expression.
   *
   * @param visitable The tensor scalar division expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  // void operator()([[maybe_unused]] tensor_scalar_div const &visitable,
  //                 [[maybe_unused]] Precedence parent_precedence) {
  //   constexpr auto precedence{Precedence::Multiplication};
  //   begin(precedence, parent_precedence);
  //   scalar_printer<StreamType> scalar_printer(m_out);
  //   apply(visitable.expr_lhs(), Precedence::Division_LHS);
  //   m_out << "/";
  //   scalar_printer.apply(visitable.expr_rhs(), Precedence::Division_RHS);
  //   end(precedence, parent_precedence);
  // }

  void operator()(
      [[maybe_unused]] tensor_to_scalar_with_tensor_mul const &visitable) {
    constexpr auto precedence{Precedence::Multiplication};
    const auto parent_precedence{m_parent_precedence};

    begin(precedence, parent_precedence);
    m_out << visitable.expr_rhs();
    m_out << "*";
    apply(visitable.expr_lhs(), precedence);
    end(precedence, parent_precedence);
  }

  // void
  // operator()([[maybe_unused]] tensor_to_scalar_with_tensor_div const
  // &visitable,
  //            [[maybe_unused]] Precedence parent_precedence) {
  //   constexpr auto precedence{Precedence::Multiplication};
  //       const auto parent_precedence{m_parent_precedence};
  //   tensor_to_scalar_printer<StreamType> printer(m_out);
  //   begin(precedence, parent_precedence);
  //   apply(visitable.expr_lhs(), Precedence::Division_LHS);
  //   m_out << "/";
  //   printer.apply(visitable.expr_rhs(), Precedence::Division_RHS);
  //   end(precedence, parent_precedence);
  // }

  void operator()(tensor_inv const &visitable) {
    print_unary("inv", visitable);
  }

  /**
   * @brief Prints a zero tensor.
   *
   * @param visitable The zero tensor to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()([[maybe_unused]] tensor_zero const &visitable) {
    m_out << "0";
  }

  void operator()(tensor_pow const &visitable) {
    m_out << "pow(";
    apply(visitable.expr_lhs(), m_parent_precedence);
    m_out << ",";
    apply(visitable.expr_rhs(), m_parent_precedence);
    m_out << ")";
  }

  // (A^2)' = (A*A)'
  // sum_r^n otimesu(pow(expr, r), pow(expr, (n-1)-r)), n := scalar expr
  void operator()(tensor_power_diff const &visitable) {
    m_out << "pow_diff(";
    apply(visitable.expr_lhs(), m_parent_precedence);
    m_out << ",";
    apply(visitable.expr_rhs(), m_parent_precedence);
    m_out << ")";
  }

  /**
   * @brief Default overload for safty reasons.
   */
  template <class T> void operator()([[maybe_unused]] T const &visitable) {
    static_assert(sizeof(T) == 0,
                  "tensor_printer: missing overload for this node type");
  }

private:
  auto apply(expression_holder<scalar_expression> const &expr,
             [[maybe_unused]] Precedence parent_precedence = Precedence::None) {
    scalar_printer<StreamType> printer(m_out);
    printer.apply(expr, parent_precedence);
  }

  using base::m_out; ///< The output stream used for printing.
  Precedence m_parent_precedence;
};

} // namespace numsim::cas
#endif // TENSOR_PRINTER_H
