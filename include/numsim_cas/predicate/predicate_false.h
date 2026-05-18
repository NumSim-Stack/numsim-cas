#ifndef PREDICATE_FALSE_H
#define PREDICATE_FALSE_H

#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/predicate/predicate_expression.h>

namespace numsim::cas {

// The constant `false` predicate. Used as the trivially-unsatisfied branch
// of an if_then_else, or as the result of folding a constant comparison
// like `5 < 2`.
class predicate_false final : public predicate_node_base_t<predicate_false> {
public:
  using base = predicate_node_base_t<predicate_false>;

  predicate_false() {}
  predicate_false(predicate_false &&data) noexcept
      : base(static_cast<base &&>(data)) {}
  predicate_false(predicate_false const &data)
      : base(static_cast<base const &>(data)) {}
  ~predicate_false() override = default;
  const predicate_false &operator=(predicate_false &&) = delete;

  friend inline bool operator<(predicate_false const &lhs,
                               predicate_false const &rhs) {
    return lhs.hash_value() < rhs.hash_value();
  }
  friend inline bool operator>(predicate_false const &lhs,
                               predicate_false const &rhs) {
    return rhs < lhs;
  }
  friend inline bool operator==(predicate_false const &lhs,
                                predicate_false const &rhs) {
    return lhs.hash_value() == rhs.hash_value();
  }
  friend inline bool operator!=(predicate_false const &lhs,
                                predicate_false const &rhs) {
    return !(lhs == rhs);
  }

private:
  void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }
};

} // namespace numsim::cas

#endif // PREDICATE_FALSE_H
