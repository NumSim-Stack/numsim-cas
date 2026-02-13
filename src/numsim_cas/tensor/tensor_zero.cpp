#include <numsim_cas/core/hash_functions.h>
#include <numsim_cas/tensor/tensor_zero.h>

namespace numsim::cas {

void tensor_zero::update_hash_value() const {
  hash_combine(base::m_hash_value, base::get_id());
}

bool operator<(tensor_zero const &lhs, tensor_zero const &rhs) {
  return lhs.hash_value() < rhs.hash_value();
}

bool operator>(tensor_zero const &lhs, tensor_zero const &rhs) {
  return rhs < lhs;
}

bool operator==(tensor_zero const &lhs, tensor_zero const &rhs) {
  return lhs.hash_value() == rhs.hash_value();
}

bool operator!=(tensor_zero const &lhs, tensor_zero const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas
