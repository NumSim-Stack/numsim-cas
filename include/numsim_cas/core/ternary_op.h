#ifndef TERNARY_OP_H
#define TERNARY_OP_H

#include <numsim_cas/core/core_fwd.h>
#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/numsim_cas_type_traits.h>
#include <numsim_cas/update_hash.h>

namespace numsim::cas {

/**
 * @class ternary_op
 * @brief A three-argument operation expression.
 *
 * Mirrors `binary_op` but with three operands — used for the
 * `if_then_else(cond, then, else)` family of nodes (#135). The
 * condition operand may have a different base than the branches
 * (e.g. for `tensor_if_then_else_scalar` where `cond` is a scalar but
 * `then`/`else` are tensors).
 *
 * @tparam ThisBase Domain-specific node base (provides expr_t).
 * @tparam BaseCond Base type of the condition operand.
 * @tparam BaseThen Base type of the `then` operand. Defaults to BaseCond.
 * @tparam BaseElse Base type of the `else` operand. Defaults to BaseThen.
 */
template <typename ThisBase, typename BaseCond = typename ThisBase::expr_t,
          typename BaseThen = BaseCond, typename BaseElse = BaseThen>
class ternary_op : public ThisBase {
public:
  using base = ThisBase;
  using base_expr = ThisBase;

  template <typename ExprC, typename ExprT, typename ExprE, typename... Args>
  ternary_op(ExprC &&cond, ExprT &&then_branch, ExprE &&else_branch,
             Args &&...args)
      : base(std::forward<Args>(args)...), m_cond(std::forward<ExprC>(cond)),
        m_then(std::forward<ExprT>(then_branch)),
        m_else(std::forward<ExprE>(else_branch)) {}

  ternary_op(ternary_op &&data) noexcept
      : base(std::forward<base_expr>(data)), m_cond(std::move(data.m_cond)),
        m_then(std::move(data.m_then)), m_else(std::move(data.m_else)) {}

  ternary_op(ternary_op const &data)
      : base(static_cast<base_expr const &>(data)), m_cond(data.m_cond),
        m_then(data.m_then), m_else(data.m_else) {}

  ~ternary_op() override {}

  ternary_op() = delete;
  const ternary_op &operator=(ternary_op const &) = delete;

  [[nodiscard]] inline auto const &expr_cond() const noexcept { return m_cond; }
  [[nodiscard]] inline auto const &expr_then() const noexcept { return m_then; }
  [[nodiscard]] inline auto const &expr_else() const noexcept { return m_else; }
  [[nodiscard]] inline auto &expr_cond() noexcept { return m_cond; }
  [[nodiscard]] inline auto &expr_then() noexcept { return m_then; }
  [[nodiscard]] inline auto &expr_else() noexcept { return m_else; }

protected:
  void update_hash_value() const noexcept override {
    base::m_hash_value =
        update_hash<ternary_op<ThisBase, BaseCond, BaseThen, BaseElse>>()(
            *this);
  }
  expression_holder<BaseCond> m_cond;
  expression_holder<BaseThen> m_then;
  expression_holder<BaseElse> m_else;
};

template <typename... Args>
struct update_hash<numsim::cas::ternary_op<Args...>> {
  std::size_t
  operator()(const numsim::cas::ternary_op<Args...> &expr) const noexcept {
    std::size_t seed{0};
    numsim::cas::hash_combine(seed, numsim::cas::ternary_op<Args...>::get_id());
    numsim::cas::hash_combine(seed, expr.expr_cond().get().hash_value());
    numsim::cas::hash_combine(seed, expr.expr_then().get().hash_value());
    numsim::cas::hash_combine(seed, expr.expr_else().get().hash_value());
    return seed;
  }
};

} // namespace numsim::cas

#endif // TERNARY_OP_H
