#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/tensor_to_scalar/tensor_to_scalar_one.h>

namespace numsim::cas {

void tensor_to_scalar_one::update_hash_value() const {
  hash_combine(base::m_hash_value, base::get_id());
}

bool operator<([[maybe_unused]] tensor_to_scalar_one const &lhs,
               [[maybe_unused]] tensor_to_scalar_one const &rhs) {
  return false;
}

bool operator>(tensor_to_scalar_one const &lhs,
               tensor_to_scalar_one const &rhs) {
  return rhs < lhs;
}

bool operator==([[maybe_unused]] tensor_to_scalar_one const &lhs,
                [[maybe_unused]] tensor_to_scalar_one const &rhs) {
  return true;
}

bool operator!=(tensor_to_scalar_one const &lhs,
                tensor_to_scalar_one const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas
