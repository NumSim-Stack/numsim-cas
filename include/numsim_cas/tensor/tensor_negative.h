#ifndef TENSOR_NEGATIVE_H
#define TENSOR_NEGATIVE_H

#include "../unary_op.h"

namespace numsim::cas {

template <typename ValueType>
class tensor_negative final : public unary_op<tensor_negative<ValueType>,
                                              tensor_expression<ValueType>> {
public:
  using base =
      unary_op<tensor_negative<ValueType>, tensor_expression<ValueType>>;

  template <typename Expr,
            std::enable_if_t<
                std::is_base_of_v<tensor_expression<ValueType>,
                                  std::remove_pointer_t<get_type_t<Expr>>>,
                bool> = true>
  tensor_negative(Expr &&_expr)
      : base(std::forward<Expr>(_expr), call_tensor::dim(_expr),
             call_tensor::rank(_expr)) {}

  tensor_negative() = delete;
  tensor_negative(tensor_negative const &add) = delete;
  tensor_negative(tensor_negative &&) = delete;
  ~tensor_negative() = default;
  const tensor_negative &operator=(tensor_negative &&) = delete;
};

} // namespace numsim::cas

#endif // TENSOR_NEGATIVE_H
