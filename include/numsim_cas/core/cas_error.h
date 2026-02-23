#ifndef CAS_ERROR_H
#define CAS_ERROR_H

#include <stdexcept>

namespace numsim::cas {

class cas_error : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class evaluation_error : public cas_error {
  using cas_error::cas_error;
};

class not_implemented_error : public cas_error {
  using cas_error::cas_error;
};

class invalid_expression_error : public cas_error {
  using cas_error::cas_error;
};

class internal_error : public cas_error {
  using cas_error::cas_error;
};

} // namespace numsim::cas

#endif // CAS_ERROR_H
