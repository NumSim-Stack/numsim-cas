#ifndef SCALAR_CONSTANT_H
#define SCALAR_CONSTANT_H

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

  friend bool operator==(scalar_constant const &a, scalar_constant const &b);
  friend bool operator!=(scalar_constant const &a, scalar_constant const &b);
  friend bool operator<(scalar_constant const &a, scalar_constant const &b);
  friend bool operator>(scalar_constant const &a, scalar_constant const &b);

protected:
  void update_hash_value() const noexcept override;

private:
  scalar_number m_value;
};

} // namespace numsim::cas

#endif // SCALAR_CONSTANT_H
