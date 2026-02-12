#ifndef TENSOR_EXPRESSION_H
#define TENSOR_EXPRESSION_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/expression.h>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/tensor/tensor_visitor_typedef.h>

namespace numsim::cas {

class tensor_expression : public expression {
public:
  using expr_t = tensor_expression;

  tensor_expression() = default;
  tensor_expression(std::size_t dim, std::size_t rank)
      : m_dim(dim), m_rank(rank) {}
  tensor_expression(tensor_expression &&data, std::size_t dim, std::size_t rank)
      : expression(std::move(static_cast<expression &&>(data))), m_dim(dim),
        m_rank(rank) {}
  tensor_expression(tensor_expression const &data, std::size_t dim,
                    std::size_t rank)
      : expression(static_cast<expression const &>(data)), m_dim(dim),
        m_rank(rank) {}
  tensor_expression(tensor_expression const &data)
      : expression(static_cast<expression const &>(data)), m_dim(data.m_dim),
        m_rank(data.m_rank) {}
  tensor_expression(tensor_expression &&data)
      : expression(std::move(static_cast<expression &&>(data))),
        m_dim(data.m_dim), m_rank(data.m_rank) {}
  virtual ~tensor_expression() = default;

  const tensor_expression &operator=(tensor_expression const &) = delete;

  [[nodiscard]] constexpr inline const auto &dim() const noexcept {
    return m_dim;
  }

  [[nodiscard]] constexpr inline const auto &rank() const noexcept {
    return m_rank;
  }

protected:
  std::size_t m_dim;
  std::size_t m_rank;
};

// std::ostream &operator<<(std::ostream &os,
//                          expression_holder<tensor_expression> const &expr) {
//   tensor_printer<std::ostream> printer(os);
//   printer.apply(expr);
//   return os;
// }

expression_holder<tensor_expression>
tag_invoke(detail::neg_fn, std::type_identity<tensor_expression>,
           expression_holder<tensor_expression> const &e);
} // namespace numsim::cas

#endif // TENSOR_EXPRESSION_H
