#ifndef SCALAR_H
#define SCALAR_H

#include <numsim_cas/core/symbol_base.h>
#include <numsim_cas/scalar/scalar_expression.h>

namespace numsim::cas {

class scalar final : public symbol_base<scalar_node_base_t<scalar>> {
public:
  using base_t = symbol_base<scalar_node_base_t<scalar>>;
  using expr_t = typename base_t::expr_t;

  explicit scalar(std::string const &name) : base_t(name) {}

  explicit scalar(scalar const &data) = delete;

  explicit scalar(scalar &&data)
      : base_t(std::move(static_cast<base_t &&>(data))) {}

  using base_t::operator=;

  const scalar &operator=(scalar const &data) {
    this->m_name = data.name();
    return *this;
  }

  const scalar &operator=(scalar &&data) {
    this->m_name = data.name();
    return *this;
  }
};

} // namespace numsim::cas

#endif // SCALAR_H
