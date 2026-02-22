#ifndef TENSOR_NEGATIVE_H
#define TENSOR_NEGATIVE_H

#include <numsim_cas/core/unary_op.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

class tensor_negative final
    : public unary_op<tensor_node_base_t<tensor_negative>> {
public:
  using base = unary_op<tensor_node_base_t<tensor_negative>>;

  template <typename Expr,
            std::enable_if_t<
                std::is_base_of_v<tensor_expression,
                                  std::remove_pointer_t<get_type_t<Expr>>>,
                bool> = true>
  tensor_negative(Expr &&_expr)
      : base(std::forward<Expr>(_expr), _expr.get().dim(), _expr.get().rank()) {
    if (auto const &sp = this->expr().get().space())
      this->set_space(*sp);
  }

  tensor_negative() = delete;
  tensor_negative(tensor_negative const &add) = delete;
  tensor_negative(tensor_negative &&) = delete;
  ~tensor_negative() override = default;
  const tensor_negative &operator=(tensor_negative &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_NEGATIVE_H
