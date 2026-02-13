#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/scalar/scalar_constant.h>

namespace numsim::cas {

void scalar_constant::update_hash_value() const noexcept {
  this->m_hash_value = 0;
  hash_combine(this->m_hash_value, this->id());
  std::visit([&](auto const &x) { hash_combine(this->m_hash_value, x); },
             m_value.raw());
}

bool operator==(scalar_constant const &a, scalar_constant const &b) {
  return a.value() == b.value();
}

bool operator!=(scalar_constant const &a, scalar_constant const &b) {
  return !(a == b);
}

bool operator<(scalar_constant const &a, scalar_constant const &b) {
  return a.value() < b.value();
}

bool operator>(scalar_constant const &a, scalar_constant const &b) {
  return b < a;
}

} // namespace numsim::cas
