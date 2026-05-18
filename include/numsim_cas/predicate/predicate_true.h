#ifndef PREDICATE_TRUE_H
#define PREDICATE_TRUE_H

#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/predicate/predicate_expression.h>

namespace numsim::cas {

// The constant `true` predicate. Used as the trivially-satisfied branch
// of an if_then_else, or as the result of folding a constant comparison
// like `2 < 3`.
class predicate_true final : public predicate_node_base_t<predicate_true> {
public:
  using base = predicate_node_base_t<predicate_true>;

  predicate_true() {}
  predicate_true(predicate_true &&data) noexcept
      : base(static_cast<base &&>(data)) {}
  predicate_true(predicate_true const &data)
      : base(static_cast<base const &>(data)) {}
  ~predicate_true() override = default;
  const predicate_true &operator=(predicate_true &&) = delete;

  friend inline bool operator<(predicate_true const &lhs,
                               predicate_true const &rhs) {
    return lhs.hash_value() < rhs.hash_value();
  }
  friend inline bool operator>(predicate_true const &lhs,
                               predicate_true const &rhs) {
    return rhs < lhs;
  }
  friend inline bool operator==(predicate_true const &lhs,
                                predicate_true const &rhs) {
    return lhs.hash_value() == rhs.hash_value();
  }
  friend inline bool operator!=(predicate_true const &lhs,
                                predicate_true const &rhs) {
    return !(lhs == rhs);
  }

private:
  void update_hash_value() const override {
    hash_combine(base::m_hash_value, base::get_id());
  }
};

} // namespace numsim::cas

#endif // PREDICATE_TRUE_H
