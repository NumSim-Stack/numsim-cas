
#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/scalar/scalar_zero.h>

namespace numsim::cas {

void scalar_zero::update_hash_value() const {
  hash_combine(base::m_hash_value, base::get_id());
}

bool operator<(scalar_zero const &lhs, scalar_zero const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

bool operator>(scalar_zero const &lhs, scalar_zero const &rhs) {
  return rhs < lhs;
}

bool operator==(scalar_zero const &lhs, scalar_zero const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

bool operator!=(scalar_zero const &lhs, scalar_zero const &rhs) {
  return !(lhs == rhs);
}
} // namespace numsim::cas
