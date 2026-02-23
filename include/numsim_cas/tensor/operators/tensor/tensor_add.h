#ifndef TENSOR_ADD_H
#define TENSOR_ADD_H

#include <numsim_cas/core/n_ary_tree.h>
#include <numsim_cas/tensor/tensor_expression.h>

namespace numsim::cas {

class tensor_add final : public n_ary_tree<tensor_node_base_t<tensor_add>> {
public:
  using base = n_ary_tree<tensor_node_base_t<tensor_add>>;
  using expr_holder_t = expression_holder<tensor_expression>;

  tensor_add(std::size_t dim, std::size_t rank) : base(dim, rank) {}
  tensor_add(tensor_add const &add)
      : base(static_cast<const base &>(add), add.dim(), add.rank()) {
    if (auto const &sp = add.space())
      this->set_space(*sp);
  }
  tensor_add(tensor_add &&add) noexcept
      : base(static_cast<base &&>(add), add.dim(), add.rank()) {
    if (auto const &sp = add.space())
      this->set_space(*sp);
  }
  ~tensor_add() override = default;
  const tensor_add &operator=(tensor_add &&) = delete;

  /// Override push_back to maintain a running space join.
  inline void push_back(expr_holder_t const &expr) {
    join_child_space(expr.get());
    base::push_back(expr);
  }

  inline void push_back(expr_holder_t &&expr) {
    join_child_space(expr.get());
    base::push_back(std::move(expr));
  }

private:
  void join_child_space(tensor_expression const &child) {
    auto const &child_sp = child.space();
    if (this->size() == 0) {
      if (child_sp)
        this->set_space(*child_sp);
    } else if (this->space()) {
      if (child_sp) {
        if (auto joined = join_tensor_space(*this->space(), *child_sp))
          this->set_space(*joined);
        else
          this->clear_space();
      } else {
        this->clear_space();
      }
    }
  }
};

} // NAMESPACE numsim::cas

#endif // TENSOR_ADD_H
