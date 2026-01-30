#ifndef TENSOR_EXPRESSION_H
#define TENSOR_EXPRESSION_H

#include "../expression.h"
#include "../expression_holder.h"
#include "visitors/tensor_printer.h"
#include <cstdlib>

namespace numsim::cas {

class tensor_expression : public expression {
public:
  using expr_type = tensor_expression;
  // using node_type = tensor_node<value_type>;

  tensor_expression(std::size_t dim, std::size_t rank)
      : m_dim(dim), m_rank(rank) {}

  tensor_expression() = default;
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

  constexpr inline auto operator-() const {
    // TODO check if this is already negative
    return make_expression<tensor_expression, tensor_negative>(this);
  }

  // tensor_space_manager m_space{};

protected:
  std::size_t m_dim;
  std::size_t m_rank;
};

std::ostream &operator<<(std::ostream &os,
                         expression_holder<tensor_expression> const &expr) {
  tensor_printer<std::ostream> printer(os);
  printer.apply(expr);
  return os;
}

struct expression_details<tensor_expression> {
  template <typename Expr> static inline auto negative(Expr &&expr) {
    return make_expression<tensor_negative>(std::forward<Expr>(expr));
  }
};

} // namespace numsim::cas

#endif // TENSOR_EXPRESSION_H
