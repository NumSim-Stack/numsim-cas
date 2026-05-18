#include <numsim_cas/basic_functions.h>
#include <numsim_cas/predicate/predicate_false.h>
#include <numsim_cas/predicate/predicate_globals.h>
#include <numsim_cas/predicate/predicate_true.h>

namespace numsim::cas {

expression_holder<predicate_expression> const &get_predicate_true() noexcept {
  static auto t = make_expression<predicate_true>();
  return t;
}

expression_holder<predicate_expression> const &get_predicate_false() noexcept {
  static auto f = make_expression<predicate_false>();
  return f;
}

} // namespace numsim::cas
