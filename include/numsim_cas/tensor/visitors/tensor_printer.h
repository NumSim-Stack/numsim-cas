#ifndef TENSOR_PRINTER_H
#define TENSOR_PRINTER_H

#include "../../numsim_cas_type_traits.h"
#include "../../printer_base.h"
#include "../../scalar/visitors/scalar_printer.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

namespace numsim::cas {

/**
 * @brief Class for printing scalar expressions with correct precedence and
 * formatting.
 *
 * @tparam ValueType The type of the scalar values.
 * @tparam StreamType The type of the output stream.
 */
template <typename ValueType, typename StreamType>
class tensor_printer final
    : public printer_base<tensor_printer<ValueType, StreamType>, StreamType> {
public:
  using base = printer_base<tensor_printer<ValueType, StreamType>, StreamType>;
  using expr_t = expression_holder<tensor_expression<ValueType>>;
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
  auto apply(expression_holder<tensor_expression<ValueType>> const &expr,
             Precedence parent_precedence = Precedence::None) {
    if (expr.is_valid()) {
      std::visit([this, parent_precedence](
                     auto &&arg) { (*this)(arg, parent_precedence); },
                 *expr);
    }
  }

  /**
   * @brief Prints a tensor value.
   *
   * @param visitable The tensor value to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(tensor<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) {
    m_out << visitable.name();
  }

  void operator()([[maybe_unused]] tensor_identity<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) {
    m_out << "tensor_identity{" << visitable.dim() << " " << visitable.rank()
          << "}";
  }

  /**
   * @brief Prints a tensor addition expression.
   *
   * @param visitable The tensor addition expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(tensor_add<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Addition};
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
      if (first && !is_same<tensor_negative<ValueType>>(child)) {
        m_out << "+";
      }
      apply(child, precedence);
      first = true;
    }

    end(precedence, parent_precedence);
  }

  void operator()(tensor_mul<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Multiplication};
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
  void operator()(tensor_negative<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Negative};
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
  void operator()(inner_product_wrapper<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Multiplication};
    auto indices_temp_lhs{visitable.indices_lhs()};
    std::for_each(std::begin(indices_temp_lhs), std::end(indices_temp_lhs),
                  [](auto &el) { el += 1; });
    auto indices_temp_rhs{visitable.indices_rhs()};
    std::for_each(std::begin(indices_temp_rhs), std::end(indices_temp_rhs),
                  [](auto &el) { el += 1; });
    // single contraction
    if (indices_temp_lhs.size() == 1 && indices_temp_rhs.size() == 1) {
      const auto rank_lhs{call_tensor::rank(visitable.expr_lhs())};
      if (indices_temp_lhs[0] == rank_lhs && indices_temp_rhs[0] == 1) {
        apply(visitable.expr_lhs(), precedence);
        m_out << "*";
        apply(visitable.expr_rhs(), precedence);
        return;
      }
    }
    // double contraction
    if (indices_temp_lhs.size() == 2 && indices_temp_rhs.size() == 2) {
      const auto rank_lhs{call_tensor::rank(visitable.expr_lhs())};
      if (indices_temp_lhs == sequence{rank_lhs - 1, rank_lhs} &&
          indices_temp_rhs == sequence{1, 2}) {
        apply(visitable.expr_lhs(), precedence);
        m_out << ":";
        apply(visitable.expr_rhs(), precedence);
        return;
      }
    }

    // fourth contraction
    if (indices_temp_lhs.size() == 4 && indices_temp_rhs.size() == 4) {
      const auto rank_lhs{call_tensor::rank(visitable.expr_lhs())};
      if (indices_temp_lhs ==
              sequence{rank_lhs - 3, rank_lhs - 2, rank_lhs - 1, rank_lhs} &&
          indices_temp_rhs == sequence{1, 2, 3, 4}) {
        apply(visitable.expr_lhs(), precedence);
        m_out << "::";
        apply(visitable.expr_rhs(), precedence);
        return;
      }
    }

    m_out << "inner(";
    apply(visitable.expr_lhs(), precedence);
    m_out << ", [";
    base::print_sequence(m_out, indices_temp_lhs, ',');
    m_out << "]";
    m_out << ", ";
    apply(visitable.expr_rhs(), precedence);
    m_out << ", [";
    base::print_sequence(m_out, indices_temp_rhs, ',');
    m_out << "]";
    m_out << ")";
  }

  /**
   * @brief Prints a basis change expression.
   *
   * @param visitable The basis change expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(basis_change_imp<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) {
    auto indices_temp{visitable.indices()};
    std::for_each(std::begin(indices_temp), std::end(indices_temp),
                  [](auto &el) { el += 1; });
    if (indices_temp == std::vector<std::size_t>{2, 1}) {
      m_out << "trans(";
      apply(visitable.expr(), parent_precedence);
      m_out << ")";
    } else {
      m_out << "permute_indices(";
      apply(visitable.expr(), parent_precedence);
      m_out << ", [";
      base::print_sequence(m_out, indices_temp, ',');
      m_out << "]";
      m_out << ")";
    }
  }

  /**
   * @brief Prints an outer product.
   *
   * @param visitable The outer product expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(outer_product_wrapper<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) {
    auto indices_temp_lhs{visitable.indices_lhs()};
    std::for_each(std::begin(indices_temp_lhs), std::end(indices_temp_lhs),
                  [](auto &el) { el += 1; });
    auto indices_temp_rhs{visitable.indices_rhs()};
    std::for_each(std::begin(indices_temp_rhs), std::end(indices_temp_rhs),
                  [](auto &el) { el += 1; });

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
      m_out << ", [";
      base::print_sequence(m_out, indices_temp_lhs, ',');
      m_out << "]";
      m_out << ", ";
      apply(visitable.expr_rhs(), Precedence::None);
      m_out << ", [";
      base::print_sequence(m_out, indices_temp_rhs, ',');
      m_out << "]";
      m_out << ")";
    }
  }

  void
  operator()([[maybe_unused]] simple_outer_product<ValueType> const &visitable,
             [[maybe_unused]] Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Multiplication};
    begin(precedence, parent_precedence);
    bool first = true;
    if (visitable.coeff().is_valid()) {
      apply(visitable.coeff(), precedence);
      m_out << "*";
    }
    m_out << "outer(";
    for (auto &child : visitable.hash_map() | std::views::values) {
      if (!first)
        m_out << ",";
      apply(child, precedence);
      first = false;
    }
    end(precedence, parent_precedence);
  }

  /**
   * @brief Prints a Kronecker delta expression.
   *
   * @param visitable The Kronecker delta expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()([[maybe_unused]] kronecker_delta<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) {
    m_out << "I";
  }

  //  /**
  //   * @brief Prints a simple outer product expression.
  //   *
  //   * @param visitable The simple outer product expression to be printed.
  //   * @param parent_precedence The precedence of the parent expression.
  //   */
  //  void operator()(simple_outer_product<ValueType> &visitable, Precedence
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
  void
  operator()([[maybe_unused]] tensor_scalar_mul<ValueType> const &visitable,
             [[maybe_unused]] Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Multiplication};
    begin(precedence, parent_precedence);
    scalar_printer<ValueType, StreamType> scalar_printer(m_out);
    scalar_printer.apply(visitable.expr_lhs(), precedence);
    m_out << "*";
    apply(visitable.expr_rhs(), precedence);
    end(precedence, parent_precedence);
  }

  /**
   * @brief Prints a tensor scalar division expression.
   *
   * @param visitable The tensor scalar division expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void
  operator()([[maybe_unused]] tensor_scalar_div<ValueType> const &visitable,
             [[maybe_unused]] Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Multiplication};
    begin(precedence, parent_precedence);
    apply(visitable.expr_lhs(), Precedence::Division_LHS);
    m_out << "/";
    apply(visitable.expr_rhs(), Precedence::Division_RHS);
    end(precedence, parent_precedence);
  }

  /**
   * @brief Prints a tensor deviatoric expression.
   *
   * @param visitable The tensor deviatoric expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(tensor_deviatoric<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) {
    print_unary("dev", visitable);
  }

  /**
   * @brief Prints a tensor symmetry expression.
   *
   * @param visitable The tensor symmetry expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(tensor_symmetry<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) {
    print_unary("sym", visitable);
  }

  /**
   * @brief Prints a zero tensor.
   *
   * @param visitable The zero tensor to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()([[maybe_unused]] tensor_zero<ValueType> const &visitable,
                  [[maybe_unused]] Precedence parent_precedence) {
    m_out << "0";
  }

  void operator()(tensor_pow<ValueType> const &visitable,
                  Precedence parent_precedence) {
    m_out << "pow(";
    apply(visitable.expr_lhs(), parent_precedence);
    m_out << ",";
    apply(visitable.expr_rhs(), parent_precedence);
    m_out << ")";
  }

private:
  auto apply(expression_holder<scalar_expression<ValueType>> const &expr,
             [[maybe_unused]] Precedence parent_precedence = Precedence::None) {
    scalar_printer<ValueType, StreamType> printer(m_out);
    printer.apply(expr, parent_precedence);
  }

  using base::m_out; ///< The output stream used for printing.
};

} // namespace numsim::cas
#endif // TENSOR_PRINTER_H
