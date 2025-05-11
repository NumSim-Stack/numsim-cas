#ifndef TENSOR_PRINTER_H
#define TENSOR_PRINTER_H

#include "../printer_base.h"
#include "../symTM_type_traits.h"
#include "../scalar/visitors/scalar_printer.h"
#include <string>
#include <algorithm>
#include <functional>
#include <iostream>
#include <variant>
#include <vector>


namespace numsim::cas {

// Assuming other required definitions for expressions and Precedence are provided
/**
 * @brief Class for printing scalar expressions with correct precedence and formatting.
 *
 * @tparam ValueType The type of the scalar values.
 * @tparam StreamType The type of the output stream.
 */
template <typename ValueType, typename StreamType>
class tensor_printer final : public printer_base<tensor_printer<ValueType, StreamType>, StreamType> {
public:
  using base = printer_base<tensor_printer<ValueType, StreamType>, StreamType>;
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
  auto apply(expression_holder<tensor_expression<ValueType>> const&expr,
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
  void operator()(tensor<ValueType> const&visitable, [[maybe_unused]] Precedence parent_precedence) {
    m_out << visitable.name();
  }

  /**
   * @brief Prints a tensor addition expression.
   *
   * @param visitable The tensor addition expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(tensor_add<ValueType> const&visitable, [[maybe_unused]]Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Addition};
    begin(precedence, parent_precedence);
    bool first = true;
    for (auto &child : visitable.hash_map() | std::views::values) {
      if (!first)
        m_out << "+";
      apply(child, precedence);
      first = false;
    }
    //
//    if (visitable.coeff().is_valid()) {
//      apply(visitable.coeff(), precedence);
//      m_out << "+";
//    }
//    for (auto &child : visitable) {

//    }

    end(precedence, parent_precedence);
  }

  void operator()(tensor_mul<ValueType> const&visitable, [[maybe_unused]]Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Multiplication};
    begin(precedence, parent_precedence);
    bool first = true;
    for (auto &child : visitable.hash_map() | std::views::values) {
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
  void operator()(tensor_negative<ValueType> const&visitable, [[maybe_unused]] Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Unary};

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
  void operator()(inner_product_wrapper<ValueType> const&visitable, [[maybe_unused]] Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Multiplication};
    m_out << "inner_product(";
    apply(visitable.expr_lhs(), precedence);
    m_out << ", ";
    apply(visitable.expr_rhs(), precedence);
    m_out << ")";
  }

  /**
   * @brief Prints a basis change expression.
   *
   * @param visitable The basis change expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(basis_change_imp<ValueType> const&visitable, [[maybe_unused]] Precedence parent_precedence) {
    m_out << "basis_change(";
    apply(visitable.expr(), parent_precedence);
    m_out << ")";
  }

  /**
   * @brief Prints an outer product.
   *
   * @param visitable The outer product expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(outer_product_wrapper<ValueType> const&visitable, [[maybe_unused]] Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Multiplication};
    m_out << "outer_product(";
    apply(visitable.expr_lhs(), precedence);
    m_out << ", ";
    apply(visitable.expr_rhs(), precedence);
    m_out << ")";
  }

  /**
   * @brief Prints a Kronecker delta expression.
   *
   * @param visitable The Kronecker delta expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()([[maybe_unused]] kronecker_delta<ValueType> const&visitable, [[maybe_unused]] Precedence parent_precedence) {
    m_out << "I";
  }

//  /**
//   * @brief Prints a simple outer product expression.
//   *
//   * @param visitable The simple outer product expression to be printed.
//   * @param parent_precedence The precedence of the parent expression.
//   */
//  void operator()(simple_outer_product<ValueType> &visitable, Precedence parent_precedence) {
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
  void operator()([[maybe_unused]] tensor_scalar_mul<ValueType> const&visitable, [[maybe_unused]] Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Multiplication};
    scalar_printer<ValueType, StreamType> scalar_printer(m_out);
    scalar_printer.apply(visitable.expr_lhs(), parent_precedence);
    //m_out << visitable.expr_lhs();
    m_out << "*";
    apply(visitable.expr_rhs(), precedence);
  }

  /**
   * @brief Prints a tensor scalar division expression.
   *
   * @param visitable The tensor scalar division expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()([[maybe_unused]] tensor_scalar_div<ValueType> const&visitable, [[maybe_unused]] Precedence parent_precedence) {
    constexpr auto precedence{Precedence::Multiplication};
    begin(precedence, parent_precedence);
    apply(visitable.expr_lhs(), Precedence::Division_LHS);
    m_out << "/";
    apply(visitable.expr_rhs(), Precedence::Division_RHS);
    end(precedence, parent_precedence);
  }

  /**
   * @brief Prints a tensor symmetry expression.
   *
   * @param visitable The tensor symmetry expression to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()(tensor_symmetry<ValueType> const&visitable, [[maybe_unused]] Precedence parent_precedence) {
    m_out << "sym(";
    apply(visitable.expr(), parent_precedence);
    m_out << ")";
  }

  /**
   * @brief Prints a zero tensor.
   *
   * @param visitable The zero tensor to be printed.
   * @param parent_precedence The precedence of the parent expression.
   */
  void operator()([[maybe_unused]]tensor_zero<ValueType> const&visitable, [[maybe_unused]] Precedence parent_precedence) {
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
  auto apply(expression_holder<scalar_expression<ValueType>> const&expr,
             [[maybe_unused]]Precedence parent_precedence = Precedence::None) {
    scalar_printer<ValueType, StreamType> printer(m_out);
    printer.apply(expr, parent_precedence);
  }

  using base::m_out; ///< The output stream used for printing.
  bool m_first{false}; ///< Indicates if this is the first expression in a sequence.
};


//template <typename ValueType, typename StreamType>
//class tensor_printer final : private printer_base {
//public:
//  tensor_printer(StreamType &out) : m_out(out) {}
//  tensor_printer(tensor_printer const &) = delete;
//  tensor_printer(tensor_printer &&) = delete;
//  const tensor_printer &operator=(tensor_printer const &) = delete;

//  void apply(expression_holder<tensor_expression<ValueType>> && expr, bool first = false) {
//    if(expr){
//      this->apply(expr.get(), first);
//    }
//  }

//  void apply(expression_holder<tensor_expression<ValueType>> & expr, bool first = false) {
//    if(expr){
//      this->apply(expr.get(), first);
//    }
//  }

//  void apply(tensor_expression<ValueType> &expr, bool first = false) {
////      auto &expr_ = static_cast<VisitableTensor_t<ValueType> &>(expr);
////      m_first = first;
////      expr_.accept(*this);
//  }

//  virtual void operator()(tensor<ValueType> &visitable) {
//    const bool sym_expr{visitable};
//    if (m_first && sym_expr) {
//      m_stream << visitable.name() << " = ";
//    }
//    if (m_first && sym_expr) {
//      m_first = false;
//      m_stream << "(";
//      //static_cast<VisitableTensor_t<ValueType> &>(visitable.expr()).accept(*this);
//      m_stream << ")";
//    } else {
//      m_stream << visitable.name();
//    }
//  }

//  virtual void operator()(tensor_add<ValueType> &visitable) {
////    std::vector<char> negative(visitable.size());
////    const auto size{visitable.size()};
////    std::size_t idx{0};
////    for (auto& child : visitable) {
////      negative[idx++] = is_same<tensor_negative<ValueType>>(child.get());
////    }
////    idx = 0;
////    for (auto& child : visitable) {
////      static_cast<VisitableTensor_t<ValueType> &>(child.get()).accept(*this);
////      if (idx++ < (size - 1)) {
////        if(!negative[idx]){
////          m_stream << "+";
////        }
////      }
////    }
//  }

//  virtual void operator()(tensor_negative<ValueType> &visitable) {
////    auto& expr{visitable.expr().get()};
////    if(is_same<tensor_add<ValueType>>(expr)){
////      m_stream << "-(";
////      static_cast<VisitableTensor_t<ValueType> &>(expr).accept(*this);
////      m_stream << ")";
////    }else{
////      m_stream << "-";
////      static_cast<VisitableTensor_t<ValueType> &>(expr).accept(*this);
////    }
//  }

//  virtual void operator()(inner_product_wrapper<ValueType> &visitable) {
//    auto& lhs{visitable.expr_lhs()};
//    auto& rhs{visitable.expr_rhs()};
//    const auto &seq_lhs{visitable.sequence_lhs()};
//    const auto &seq_rhs{visitable.sequence_rhs()};
//    m_stream << "inner_product<<";
//    m_sepperator = "";
//    std::for_each(seq_lhs.begin(), seq_lhs.end(), print_seq);
//    m_stream << ">,<";
//    m_sepperator = "";
//    std::for_each(seq_rhs.begin(), seq_rhs.end(), print_seq);
//    m_stream << ">>(";
//    //static_cast<VisitableTensor_t<ValueType> &>(lhs.get()).accept(*this);
//    m_stream << ",";
//    //static_cast<VisitableTensor_t<ValueType> &>(rhs.get()).accept(*this);
//    m_stream << ")";
//  }

//  virtual void operator()(basis_change_imp<ValueType> &visitable) {
//    auto& expr{visitable.expr()};
//    const auto &seq{visitable.indices()};
//    m_stream << "basis_change<";
//    m_sepperator = "";
//    std::for_each(seq.begin(), seq.end(), print_seq);
//    m_stream << ">(";
//    //static_cast<VisitableTensor_t<ValueType> &>(expr.get()).accept(*this);
//    m_stream << ")";
//  }

//  virtual void operator()(outer_product_wrapper<ValueType> &visitable) {
//    auto& lhs{visitable.expr_lhs()};
//    auto& rhs{visitable.expr_rhs()};
//    const auto &seq_lhs{visitable.sequence_lhs()};
//    const auto &seq_rhs{visitable.sequence_rhs()};
//    m_stream << "outer_product<<";
//    m_sepperator = "";
//    std::for_each(seq_lhs.begin(), seq_lhs.end(), print_seq);
//    m_stream << ">,<";
//    m_sepperator = "";
//    std::for_each(seq_rhs.begin(), seq_rhs.end(), print_seq);
//    m_stream << ">>(";
//    //static_cast<VisitableTensor_t<ValueType> &>(lhs.get()).accept(*this);
//    m_stream << ",";
//    //static_cast<VisitableTensor_t<ValueType> &>(rhs.get()).accept(*this);
//    m_stream << ")";
//  }

//  virtual void operator()(kronecker_delta<ValueType> &visitable) { m_stream << "I"; }

//  virtual void operator()(simple_outer_product<ValueType> &visitable) {
//    m_stream << "simple_outer(";
//    for (auto& child : visitable) {
//      //static_cast<VisitableTensor_t<ValueType> &>(child.get()).accept(*this);
//      if(&child != &visitable.back()){
//        m_stream << ", ";
//      }
//    }
//    m_stream << ")";
//  }

//  virtual void operator()(tensor_scalar_mul<ValueType> &) {}

//  virtual void operator()(tensor_scalar_div<ValueType> &visitable) {
//    m_stream << "(";
//    //static_cast<VisitableTensor_t<ValueType> &>(visitable.expr_rhs().get()).accept(*this);
//    m_stream << ")/";
//    m_stream << "(";
//    scalar_printer<ValueType, Ostream> scalar_print(m_stream);
//    scalar_print.apply(visitable.expr_lhs().get());
//    m_stream << ")";
//  }

//  virtual void operator()(tensor_symmetry<ValueType> &visitable) {
//    m_stream << "sym(";
//    //static_cast<VisitableTensor_t<ValueType> &>(visitable.expr().get()).accept(*this);
//    m_stream << ")";
//  }

//private:
//  template <typename Type, typename Func> void loop(Type &type, Func sep) {
//    const auto size{type.size()};
//    std::size_t idx{0};
//    for (auto& child : type) {
//      //static_cast<VisitableTensor_t<ValueType> &>(child.get()).accept(*this);
//      if (idx++ < (size - 1)) {
//        m_stream << sep(child.get());
//      }
//    }
//  }

//  StreamType &m_out;
//  bool m_first{false};
//  std::string m_sepperator;
//  std::function<void(std::size_t)> print_seq{
//                                             [&sep = m_sepperator, &stream = m_stream](std::size_t i) {
//                                               stream << sep;
//                                               stream << i + 1;
//                                               sep = ",";
//                                             }};
//};

}
#endif // TENSOR_PRINTER_H
