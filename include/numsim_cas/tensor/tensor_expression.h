#ifndef TENSOR_EXPRESSION_H
#define TENSOR_EXPRESSION_H

#include <numsim_cas/basic_functions.h>
#include <numsim_cas/core/expression.h>
#include <numsim_cas/core/expression_holder.h>
#include <numsim_cas/tensor/tensor_space.h>
#include <numsim_cas/tensor/tensor_visitor_typedef.h>
#include <optional>

namespace numsim::cas {

class tensor_expression : public expression {
public:
  using expr_t = tensor_expression;

  tensor_expression() = default;
  tensor_expression(std::size_t dim, std::size_t rank)
      : m_dim(dim), m_rank(rank) {}
  // NOTE: 2-arg constructors intentionally do NOT copy m_tensor_space.
  // Used by n_ary_tree which constructs a fresh base and manages space
  // separately.
  tensor_expression(tensor_expression &&data, std::size_t dim, std::size_t rank)
      : expression(std::move(static_cast<expression &&>(data))), m_dim(dim),
        m_rank(rank) {}
  tensor_expression(tensor_expression const &data, std::size_t dim,
                    std::size_t rank)
      : expression(static_cast<expression const &>(data)), m_dim(dim),
        m_rank(rank) {}
  tensor_expression(tensor_expression const &data)
      : expression(static_cast<expression const &>(data)), m_dim(data.m_dim),
        m_rank(data.m_rank), m_tensor_space(data.m_tensor_space) {}
  tensor_expression(tensor_expression &&data) noexcept
      : expression(std::move(static_cast<expression &&>(data))),
        m_dim(data.m_dim), m_rank(data.m_rank),
        m_tensor_space(std::move(data.m_tensor_space)) {}
  ~tensor_expression() override = default;

  const tensor_expression &operator=(tensor_expression const &) = delete;

  [[nodiscard]] constexpr inline const auto &dim() const noexcept {
    return m_dim;
  }

  [[nodiscard]] constexpr inline const auto &rank() const noexcept {
    return m_rank;
  }

  [[nodiscard]] std::optional<tensor_space> const &space() const noexcept {
    return m_tensor_space;
  }
  void set_space(tensor_space s) noexcept { m_tensor_space = std::move(s); }
  void clear_space() noexcept { m_tensor_space.reset(); }

protected:
  std::size_t m_dim;
  std::size_t m_rank;
  std::optional<tensor_space> m_tensor_space{};
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
