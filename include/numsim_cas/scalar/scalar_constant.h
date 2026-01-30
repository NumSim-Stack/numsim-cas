#ifndef SCALAR_CONSTANT_H
#define SCALAR_CONSTANT_H

#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/core/scalar_number.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar_constant final : public scalar_node_base_t<scalar_constant> {
public:
  using base = scalar_node_base_t<scalar_constant>;
  scalar_constant() = delete;
  template <typename T>
  explicit scalar_constant(T const &v) : base(), m_value(v) {}
  scalar_constant(scalar_constant const &) = default;
  scalar_constant(scalar_constant &&) noexcept = default;
  scalar_constant &operator=(scalar_constant const &) = delete;
  scalar_constant &operator=(scalar_constant &&) noexcept = delete;
  ~scalar_constant() override = default;

  auto const &value() const noexcept { return m_value; }

protected:
  void update_hash_value() const noexcept override {
    this->m_hash_value = 0;
    hash_combine(this->m_hash_value, this->id());
    // hash_combine(this->m_hash_value, m_value.raw().index());
    std::visit([&](auto const &x) { hash_combine(this->m_hash_value, x); },
               m_value.raw());
  }

private:
  scalar_number m_value;
};

// Equality is safe (complex has ==)
inline bool operator==(scalar_constant const &a, scalar_constant const &b) {
  return a.value() == b.value();
}
inline bool operator!=(scalar_constant const &a, scalar_constant const &b) {
  return !(a == b);
}

// Define a strict weak ordering that works for complex
inline bool operator<(scalar_constant const &a, scalar_constant const &b) {
  return a.value() < b.value();
}
inline bool operator>(scalar_constant const &a, scalar_constant const &b) {
  return b < a;
}

} // namespace numsim::cas

#endif // SCALAR_CONSTANT_H
