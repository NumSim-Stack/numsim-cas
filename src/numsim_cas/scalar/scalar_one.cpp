#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/scalar/scalar_one.h>

namespace numsim::cas {

scalar_one::scalar_one() { hash_combine(this->m_hash_value, base::get_id()); }

void scalar_one::update_hash_value() const {
  hash_combine(base::m_hash_value, base::get_id());
}

bool operator<([[maybe_unused]] scalar_one const &lhs,
               [[maybe_unused]] scalar_one const &rhs) {
  return false;
}

bool operator>(scalar_one const &lhs, scalar_one const &rhs) {
  return rhs < lhs;
}

bool operator==([[maybe_unused]] scalar_one const &lhs,
                [[maybe_unused]] scalar_one const &rhs) {
  return true;
}

bool operator!=(scalar_one const &lhs, scalar_one const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas
