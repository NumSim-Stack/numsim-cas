#include <numsim_cas/scalar/scalar_one.h>

namespace numsim::cas {
bool operator<([[maybe_unused]] scalar_one const &lhs,
               [[maybe_unused]] scalar_one const &rhs) {
  return true;
}

bool operator>(scalar_one const &lhs, scalar_one const &rhs) {
  return !(lhs < rhs);
}

bool operator==([[maybe_unused]] scalar_one const &lhs,
                [[maybe_unused]] scalar_one const &rhs) {
  return true;
}

bool operator!=(scalar_one const &lhs, scalar_one const &rhs) {
  return !(lhs == rhs);
}

} // namespace numsim::cas
