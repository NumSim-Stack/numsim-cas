#include <numsim_cas/core/expression.h>

namespace numsim::cas {

expression::hash_type const &expression::hash_value() const {
  if (!m_hash_value) {
    update_hash_value();
  }
  return m_hash_value;
}

bool expression::operator==(expression const &rhs) const noexcept {
  if (this == &rhs)
    return true;

  // different dynamic node type => not equal
  if (id() != rhs.id())
    return false;

  // fast reject
  if (hash_value() != rhs.hash_value())
    return false;

  // same type => do the real compare
  return equals_same_type(rhs);
}

bool expression::operator!=(expression const &rhs) const noexcept {
  return !(*this == rhs);
}

bool expression::operator<(expression const &rhs) const noexcept {
  if (hash_value() != rhs.hash_value())
    return hash_value() < rhs.hash_value();
  if (id() != rhs.id())
    return id() < rhs.id();
  return less_than_same_type(rhs);
}

} // namespace numsim::cas
