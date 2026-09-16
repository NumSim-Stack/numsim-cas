#include <numsim_cas/core/expression.h>

#include <typeinfo>

namespace numsim::cas {

expression::hash_type const &expression::hash_value() const {
  if (!m_hash_value) {
    update_hash_value();
  }
  return m_hash_value;
}

bool expression::operator==(expression const &rhs) const {
  if (this == &rhs)
    return true;

  // different dynamic node type => not equal
  if (id() != rhs.id())
    return false;

  // fast reject
  if (hash_value() != rhs.hash_value())
    return false;

  // id() indexes the node's own domain type list, so nodes from different
  // domains share ids; the downcast in equals_same_type needs the exact type.
  if (typeid(*this) != typeid(rhs))
    return false;

  // same type => do the real compare
  return equals_same_type(rhs);
}

bool expression::operator!=(expression const &rhs) const {
  return !(*this == rhs);
}

bool expression::operator<(expression const &rhs) const {
  if (hash_value() != rhs.hash_value())
    return hash_value() < rhs.hash_value();
  if (id() != rhs.id())
    return id() < rhs.id();
  // Cross-domain ties need an order before the same-type downcast. before()
  // is arbitrary but consistent within a run, which is all a key needs.
  if (typeid(*this) != typeid(rhs))
    return typeid(*this).before(typeid(rhs));
  return less_than_same_type(rhs);
}

} // namespace numsim::cas
